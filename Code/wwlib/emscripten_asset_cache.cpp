#include "emscripten_asset_cache.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA
#include <emscripten.h>
#endif

namespace {

constexpr char kRenegadeEmscriptenManifestPath[] = "/renegade-assets-manifest.txt";
constexpr char kRenegadeEmscriptenDefaultAssetBaseUrl[] = "Renegade-assets/";
constexpr char kRenegadeEmscriptenCacheRoot[] = "/renegade-cache";
constexpr char kRenegadeEmscriptenAssetCacheRoot[] = "/renegade-cache/assets";
constexpr char kRenegadeEmscriptenUserCacheRoot[] = "/renegade-cache/user";
constexpr Sint64 kRenegadeEmscriptenRangeChunkSize = 512 * 1024;

std::string Normalize_Path_Separators(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string Fold_Path(std::string path)
{
    path = Normalize_Path_Separators(std::move(path));
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return path;
}

std::string Strip_Trailing_Path_Separators(std::string path)
{
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

std::string Append_Path_Component(const std::string & base, const std::string & component)
{
    if (base.empty()) {
        return component;
    }
    if (component.empty()) {
        return base;
    }
    if (base == "/") {
        return base + component;
    }
    if (base.back() == '/') {
        return base + component;
    }
    return base + "/" + component;
}

std::string Get_Parent_Path(const std::string & path)
{
    if (path.empty()) {
        return {};
    }

    const std::string normalized = Strip_Trailing_Path_Separators(Normalize_Path_Separators(path));
    const std::size_t separator = normalized.find_last_of('/');
    if (separator == std::string::npos) {
        return {};
    }
    if (separator == 0) {
        return "/";
    }
    return normalized.substr(0, separator);
}

std::string Collapse_Dot_Segments(const std::string & raw_path)
{
    if (raw_path.empty()) {
        return {};
    }

    const std::string path = Normalize_Path_Separators(raw_path);
    std::vector<std::string> components;
    std::size_t cursor = 0;
    const bool absolute = !path.empty() && path.front() == '/';
    if (absolute) {
        cursor = 1;
    }

    while (cursor <= path.size()) {
        const std::size_t separator = path.find('/', cursor);
        const std::string component = path.substr(cursor, separator - cursor);
        cursor = (separator == std::string::npos) ? (path.size() + 1) : (separator + 1);

        if (component.empty() || component == ".") {
            continue;
        }

        if (component == "..") {
            if (!components.empty() && components.back() != "..") {
                components.pop_back();
            } else if (!absolute) {
                components.push_back(component);
            }
            continue;
        }

        components.push_back(component);
    }

    std::string collapsed = absolute ? std::string("/") : std::string();
    for (std::size_t index = 0; index < components.size(); ++index) {
        if (!collapsed.empty() && collapsed.back() != '/') {
            collapsed += '/';
        }
        collapsed += components[index];
    }

    if (collapsed.empty()) {
        return absolute ? std::string("/") : std::string(".");
    }
    return collapsed;
}

std::string Current_Directory_Path()
{
    char * current_directory = SDL_GetCurrentDirectory();
    if (!current_directory || !*current_directory) {
        if (current_directory) {
            SDL_free(current_directory);
        }
        return "/";
    }

    std::string result(current_directory);
    SDL_free(current_directory);
    result = Normalize_Path_Separators(result);
    result = Strip_Trailing_Path_Separators(result);
    return result.empty() ? std::string("/") : result;
}

std::string Canonicalize_Request_Path(const std::string & normalized_path)
{
    if (normalized_path.empty()) {
        return {};
    }

    std::string path = Normalize_Path_Separators(normalized_path);
    if (path.empty()) {
        return {};
    }

    if (path.front() != '/') {
        path = Append_Path_Component(Current_Directory_Path(), path);
    }

    path = Collapse_Dot_Segments(path);
    while (!path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }

    if (path == ".") {
        path.clear();
    }
    return path;
}

bool Is_Mutable_Open_Mode(const char * mode)
{
    return mode != nullptr
        && (std::strchr(mode, 'w') != nullptr || std::strchr(mode, 'a') != nullptr || std::strchr(mode, '+') != nullptr);
}

bool Is_Read_Only_Open_Mode(const char * mode)
{
    return mode != nullptr
        && std::strchr(mode, 'r') != nullptr
        && std::strchr(mode, 'w') == nullptr
        && std::strchr(mode, 'a') == nullptr
        && std::strchr(mode, '+') == nullptr;
}

bool Open_Mode_Needs_Seeded_Content(const char * mode)
{
    return mode != nullptr
        && (std::strchr(mode, 'r') != nullptr || std::strchr(mode, '+') != nullptr || std::strchr(mode, 'a') != nullptr);
}

bool Starts_With(const std::string & value, const char * prefix)
{
    if (!prefix) {
        return false;
    }

    const std::size_t prefix_length = std::strlen(prefix);
    return value.size() >= prefix_length && value.compare(0, prefix_length, prefix) == 0;
}

bool Ends_With(const std::string & value, const char * suffix)
{
    if (!suffix) {
        return false;
    }

    const std::size_t suffix_length = std::strlen(suffix);
    return value.size() >= suffix_length && value.compare(value.size() - suffix_length, suffix_length, suffix) == 0;
}

bool Is_Renlog_Path(const std::string & folded_request_path)
{
    return folded_request_path.size() > 11
        && Starts_With(folded_request_path, "renlog_")
        && Ends_With(folded_request_path, ".txt");
}

std::string Canonicalize_Local_User_Data_Request_Path(const std::string & canonical_request_path)
{
    if (canonical_request_path.empty()) {
        return {};
    }

    const std::string folded_request_path = Fold_Path(canonical_request_path);
    if (folded_request_path == "server.ini"
        || folded_request_path == "banlist.txt"
        || folded_request_path == "wolbanlist.txt"
        || folded_request_path == "commands.txt"
        || folded_request_path == "sysinfo.txt"
        || folded_request_path == "history.txt"
        || folded_request_path == "_logfile.txt"
        || folded_request_path == "_asserts.txt"
        || folded_request_path == "_except.txt"
        || folded_request_path == "gameres.dat"
        || folded_request_path == "perf_log.txt"
        || folded_request_path == "profile_log.txt") {
        return folded_request_path;
    }

    if (Is_Renlog_Path(folded_request_path)) {
        return folded_request_path;
    }

    if (folded_request_path == "save" || folded_request_path == "data/save") {
        return "data/save";
    }

    if (Starts_With(folded_request_path, "save/")) {
        return Append_Path_Component("data", folded_request_path);
    }

    if (Starts_With(folded_request_path, "data/save/")) {
        return folded_request_path;
    }

    if (folded_request_path.find('/') == std::string::npos && Ends_With(folded_request_path, ".sav")) {
        return Append_Path_Component("data/save", folded_request_path);
    }

    return {};
}

bool Is_Local_User_Data_Request_Path(const std::string & canonical_request_path)
{
    return !Canonicalize_Local_User_Data_Request_Path(canonical_request_path).empty();
}

bool Should_Seed_Local_User_Data_Request_Path(const std::string &)
{
    return false;
}

bool Path_Exists_Raw(const std::string & path)
{
    if (path.empty()) {
        return false;
    }
    return SDL_GetPathInfo(path.c_str(), nullptr);
}

bool Get_Path_Info_Raw(const std::string & path, SDL_PathInfo * info)
{
    if (path.empty()) {
        return false;
    }
    return SDL_GetPathInfo(path.c_str(), info);
}

bool Ensure_Directory_Tree_Raw(const std::string & path)
{
    if (path.empty()) {
        return false;
    }

    const std::string normalized = Strip_Trailing_Path_Separators(Normalize_Path_Separators(path));
    if (normalized.empty()) {
        return false;
    }

    std::string current = (normalized.front() == '/') ? std::string("/") : std::string();
    std::size_t cursor = (normalized.front() == '/') ? 1u : 0u;

    while (cursor < normalized.size()) {
        while (cursor < normalized.size() && normalized[cursor] == '/') {
            ++cursor;
        }
        if (cursor >= normalized.size()) {
            break;
        }

        const std::size_t component_start = cursor;
        while (cursor < normalized.size() && normalized[cursor] != '/') {
            ++cursor;
        }

        const std::string component = normalized.substr(component_start, cursor - component_start);
        if (component.empty() || component == ".") {
            continue;
        }

        current = Append_Path_Component(current, component);
        SDL_PathInfo info = {};
        if (SDL_GetPathInfo(current.c_str(), &info)) {
            if (info.type != SDL_PATHTYPE_DIRECTORY) {
                return false;
            }
            continue;
        }

        if (!SDL_CreateDirectory(current.c_str())) {
            return false;
        }
    }

    return true;
}

std::string Directory_Key(const std::string & canonical_path)
{
    return Fold_Path(Strip_Trailing_Path_Separators(canonical_path));
}

struct ManifestState {
    bool loaded = false;
    std::unordered_map<std::string, std::string> files;
    std::unordered_map<std::string, std::vector<std::string>> directory_entries;
    std::unordered_set<std::string> directories;
};

ManifestState & Get_Manifest_State()
{
    static ManifestState manifest_state;
    return manifest_state;
}

std::mutex & Get_Manifest_Mutex()
{
    static std::mutex mutex;
    return mutex;
}

struct RangeFileCache {
    std::string remote_relative_path;
    Sint64 size = -1;
    std::unordered_map<Sint64, std::vector<unsigned char>> chunks;
    std::mutex mutex;
};

struct RangeStreamState {
    std::shared_ptr<RangeFileCache> file;
    Sint64 position = 0;
};

struct SyncedFileStreamState {
    SDL_IOStream * stream = nullptr;
    bool dirty = false;
};

std::unordered_map<std::string, std::shared_ptr<RangeFileCache>> & Get_Range_File_Caches()
{
    static std::unordered_map<std::string, std::shared_ptr<RangeFileCache>> caches;
    return caches;
}

std::mutex & Get_Range_File_Caches_Mutex()
{
    static std::mutex mutex;
    return mutex;
}

std::mutex & Get_Cache_Init_Mutex()
{
    static std::mutex mutex;
    return mutex;
}

bool & Cache_Initialized()
{
    static bool initialized = false;
    return initialized;
}

bool Is_Mix_Asset_Path(const std::string & canonical_relative_path)
{
    const std::string folded = Fold_Path(canonical_relative_path);
    return folded.size() >= 4 && folded.compare(folded.size() - 4, 4, ".mix") == 0;
}

std::string Asset_Cache_Path(const std::string & canonical_relative_path)
{
    return Append_Path_Component(kRenegadeEmscriptenAssetCacheRoot, canonical_relative_path);
}

std::string User_Cache_Path(const std::string & canonical_relative_path)
{
    const std::string canonical_user_data_path = Canonicalize_Local_User_Data_Request_Path(canonical_relative_path);
    const std::string &path_key = canonical_user_data_path.empty() ? canonical_relative_path : canonical_user_data_path;
    return Append_Path_Component(kRenegadeEmscriptenUserCacheRoot, Directory_Key(path_key));
}

bool Resolve_Manifest_File(const std::string & canonical_request_path, std::string & canonical_remote_path)
{
    const auto & files = Get_Manifest_State().files;
    const auto entry = files.find(Fold_Path(canonical_request_path));
    if (entry == files.end()) {
        return false;
    }
    canonical_remote_path = entry->second;
    return true;
}

bool Manifest_Has_Directory(const std::string & canonical_directory_path)
{
    const auto & manifest = Get_Manifest_State();
    return manifest.directories.find(Directory_Key(canonical_directory_path)) != manifest.directories.end();
}

bool Load_Manifest()
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    return false;
#else
    std::scoped_lock lock(Get_Manifest_Mutex());
    ManifestState & manifest = Get_Manifest_State();
    if (manifest.loaded) {
        return !manifest.files.empty();
    }

    manifest.loaded = true;
    manifest.files.clear();
    manifest.directory_entries.clear();
    manifest.directories.clear();
    manifest.directories.insert({});

    SDL_IOStream * stream = SDL_IOFromFile(kRenegadeEmscriptenManifestPath, "rb");
    if (!stream) {
        return false;
    }

    const Sint64 manifest_size = SDL_GetIOSize(stream);
    if (manifest_size < 0) {
        SDL_CloseIO(stream);
        return false;
    }

    std::string manifest_text;
    manifest_text.resize(static_cast<std::size_t>(manifest_size));
    if (!manifest_text.empty()) {
        const std::size_t bytes_read = SDL_ReadIO(stream, manifest_text.data(), manifest_text.size());
        manifest_text.resize(bytes_read);
    }
    SDL_CloseIO(stream);

    std::unordered_map<std::string, std::unordered_set<std::string>> directory_entry_sets;
    std::size_t cursor = 0;
    while (cursor < manifest_text.size()) {
        const std::size_t next_newline = manifest_text.find('\n', cursor);
        const std::size_t line_end = (next_newline == std::string::npos) ? manifest_text.size() : next_newline;
        std::string line = manifest_text.substr(cursor, line_end - cursor);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (!line.empty()) {
            line = Strip_Trailing_Path_Separators(Normalize_Path_Separators(line));
            if (!line.empty()) {
                manifest.files[Fold_Path(line)] = line;

                std::string directory;
                std::size_t component_start = 0;
                while (component_start < line.size()) {
                    const std::size_t separator = line.find('/', component_start);
                    const bool last_component = (separator == std::string::npos);
                    const std::size_t component_end = last_component ? line.size() : separator;
                    const std::string component = line.substr(component_start, component_end - component_start);
                    if (!component.empty()) {
                        directory_entry_sets[Directory_Key(directory)].insert(component);
                        if (last_component) {
                            break;
                        }

                        directory = Append_Path_Component(directory, component);
                        manifest.directories.insert(Directory_Key(directory));
                    }

                    if (last_component) {
                        break;
                    }
                    component_start = separator + 1;
                }
            }
        }

        if (next_newline == std::string::npos) {
            break;
        }
        cursor = next_newline + 1;
    }

    for (auto & entry : directory_entry_sets) {
        std::vector<std::string> children(entry.second.begin(), entry.second.end());
        std::sort(children.begin(), children.end(), [](const std::string & lhs, const std::string & rhs) {
            return Fold_Path(lhs) < Fold_Path(rhs);
        });
        manifest.directory_entries.emplace(entry.first, std::move(children));
    }

    return !manifest.files.empty();
#endif
}

bool Resolve_Manifest_Backings(const std::string & normalized_path,
    std::string & canonical_request_path,
    std::string * canonical_remote_path = nullptr,
    std::string * asset_cache_path = nullptr,
    std::string * user_cache_path = nullptr)
{
    canonical_request_path = Canonicalize_Request_Path(normalized_path);
    if (canonical_request_path.empty()) {
        return false;
    }

    if (user_cache_path) {
        *user_cache_path = User_Cache_Path(canonical_request_path);
    }

    std::string resolved_remote_path;
    const bool has_remote_asset = Load_Manifest() && Resolve_Manifest_File(canonical_request_path, resolved_remote_path);
    if (!has_remote_asset) {
        return false;
    }

    if (canonical_remote_path) {
        *canonical_remote_path = resolved_remote_path;
    }
    if (asset_cache_path) {
        *asset_cache_path = Asset_Cache_Path(resolved_remote_path);
    }
    return true;
}

#if defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA
EM_ASYNC_JS(int, renegade_emscripten_init_cache_js, (), {
    const ensureDir = (path) => {
        if (!path || path === '/') {
            return;
        }

        const parts = path.split('/');
        let current = '';
        for (const part of parts) {
            if (!part) {
                continue;
            }
            current += '/' + part;
            const analyzed = FS.analyzePath(current);
            if (!analyzed.exists) {
                FS.mkdir(current);
            }
        }
    };

    try {
        ensureDir('/renegade-cache');
        ensureDir('/renegade-cache/assets');
        ensureDir('/renegade-cache/user');

        if (!Module.renegadeAssetCacheMounted) {
            FS.mount(IDBFS, {}, '/renegade-cache');
            Module.renegadeAssetCacheMounted = true;
        }

        if (!Module.renegadeAssetCacheInitPromise) {
            Module.renegadeAssetCacheInitPromise = new Promise((resolve, reject) => {
                FS.syncfs(true, (err) => err ? reject(err) : resolve());
            });
        }

        await Module.renegadeAssetCacheInitPromise;
        return 1;
    } catch (err) {
        console.error('Renegade Emscripten asset cache initialization failed:', err);
        Module.renegadeAssetCacheInitPromise = null;
        return 0;
    }
});

EM_ASYNC_JS(int, renegade_emscripten_sync_cache_js, (), {
    try {
        if (!Module.renegadeAssetCacheInitPromise) {
            console.error('Renegade Emscripten asset cache sync requested before initialization');
            return 0;
        }

        await Module.renegadeAssetCacheInitPromise;
        await new Promise((resolve, reject) => {
            FS.syncfs(false, (err) => err ? reject(err) : resolve());
        });
        return 1;
    } catch (err) {
        console.error('Renegade Emscripten asset cache sync failed:', err);
        return 0;
    }
});

EM_ASYNC_JS(int, renegade_emscripten_fetch_asset_js, (const char * local_path_c, const char * remote_relative_c, const char * default_base_url_c), {
    const localPath = UTF8ToString(local_path_c);
    const remoteRelative = UTF8ToString(remote_relative_c);
    const defaultBaseUrl = UTF8ToString(default_base_url_c);

    const ensureDir = (path) => {
        if (!path || path === '/') {
            return;
        }

        const parts = path.split('/');
        let current = '';
        for (const part of parts) {
            if (!part) {
                continue;
            }
            current += '/' + part;
            const analyzed = FS.analyzePath(current);
            if (!analyzed.exists) {
                FS.mkdir(current);
            }
        }
    };

    const configuredBaseUrl = Module.renegadeAssetBaseUrl || globalThis.RENEGADE_ASSET_BASE_URL || defaultBaseUrl;
    const absoluteBaseUrl = new URL(configuredBaseUrl, globalThis.location.href);
    const assetUrl = new URL(remoteRelative, absoluteBaseUrl);

    try {
        const existing = FS.analyzePath(localPath);
        if (existing.exists) {
            return 1;
        }

        const lastSlash = localPath.lastIndexOf('/');
        if (lastSlash > 0) {
            ensureDir(localPath.substring(0, lastSlash));
        }

        const response = await fetch(assetUrl, { credentials: 'same-origin' });
        if (!response.ok) {
            console.error(`Renegade asset fetch failed for ${assetUrl}: ${response.status} ${response.statusText}`);
            return 0;
        }

        const bytes = new Uint8Array(await response.arrayBuffer());
        FS.writeFile(localPath, bytes, { canOwn: true });

        await new Promise((resolve, reject) => {
            FS.syncfs(false, (err) => err ? reject(err) : resolve());
        });

        return 1;
    } catch (err) {
        console.error(`Renegade asset fetch failed for ${assetUrl}:`, err);
        return 0;
    }
});

EM_JS(void, renegade_emscripten_init_range_cache_helpers_js, (), {
    if (Module.renegadeRangeCacheHelpers) {
        return;
    }

    const transactionDone = (tx) => new Promise((resolve, reject) => {
        tx.oncomplete = () => resolve();
        tx.onabort = () => reject(tx.error || new Error('IndexedDB transaction aborted'));
        tx.onerror = () => reject(tx.error || new Error('IndexedDB transaction failed'));
    });

    const normalizeBytes = (bytes) => {
        if (bytes instanceof Uint8Array) {
            return bytes;
        }
        if (bytes instanceof ArrayBuffer) {
            return new Uint8Array(bytes);
        }
        if (ArrayBuffer.isView(bytes)) {
            return new Uint8Array(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength));
        }
        return new Uint8Array();
    };

    const parseContentRangeSize = (response) => {
        const contentRange = response.headers.get('content-range') || response.headers.get('Content-Range') || '';
        const slash = contentRange.lastIndexOf('/');
        if (slash < 0) {
            return -1;
        }

        const totalSize = Number.parseInt(contentRange.substring(slash + 1), 10);
        return Number.isFinite(totalSize) && totalSize >= 0 ? totalSize : -1;
    };

    const persistedChunkPartSize = 64 * 1024;
    const chunkMetaStoreName = 'chunk_meta';
    const chunkPartStoreName = 'chunk_parts';
    const chunkMetaKey = (assetKey, chunkIndex) => `${assetKey}\nmeta\n${chunkIndex}`;
    const chunkPartKey = (assetKey, chunkIndex, partIndex) => `${assetKey}\npart\n${chunkIndex}\n${partIndex}`;

    const openDb = () => {
        if (!Module.renegadeRangeCacheDbPromise) {
            Module.renegadeRangeCacheDbPromise = new Promise((resolve, reject) => {
                const request = globalThis.indexedDB.open('renegade-range-cache', 1);
                request.onupgradeneeded = () => {
                    const db = request.result;
                    if (!db.objectStoreNames.contains('files')) {
                        db.createObjectStore('files', { keyPath: 'key' });
                    }
                    if (!db.objectStoreNames.contains(chunkMetaStoreName)) {
                        db.createObjectStore(chunkMetaStoreName, { keyPath: 'key' });
                    }
                    if (!db.objectStoreNames.contains(chunkPartStoreName)) {
                        db.createObjectStore(chunkPartStoreName, { keyPath: 'key' });
                    }
                };
                request.onsuccess = () => resolve(request.result);
                request.onerror = () => reject(request.error || new Error('Failed to open IndexedDB range cache'));
            });
        }
        return Module.renegadeRangeCacheDbPromise;
    };

    const deleteRecords = async (storeName, keys) => {
        if (!keys.length) {
            return;
        }

        const db = await openDb();
        const tx = db.transaction(storeName, 'readwrite');
        const store = tx.objectStore(storeName);
        for (const key of keys) {
            store.delete(key);
        }
        await transactionDone(tx);
    };

    const deleteChunkRecords = async (assetKey, chunkIndex, partCount) => {
        const db = await openDb();
        const tx = db.transaction([chunkMetaStoreName, chunkPartStoreName], 'readwrite');
        tx.objectStore(chunkMetaStoreName).delete(chunkMetaKey(assetKey, chunkIndex));
        if (Number.isInteger(partCount) && partCount > 0) {
            const partStore = tx.objectStore(chunkPartStoreName);
            for (let partIndex = 0; partIndex < partCount; ++partIndex) {
                partStore.delete(chunkPartKey(assetKey, chunkIndex, partIndex));
            }
        }
        await transactionDone(tx);
    };

    const dropUnreadableRecord = (storeName, key, err) => {
        console.warn(`Renegade range cache dropped unreadable ${storeName} entry for ${key}:`, err);
        void deleteRecords(storeName, [key]).catch((deleteErr) => {
            console.warn(`Renegade range cache failed to delete unreadable ${storeName} entry for ${key}:`, deleteErr);
        });
    };

    const dropUnreadableChunk = (assetKey, chunkIndex, partCount, err) => {
        console.warn(`Renegade range cache dropped unreadable chunk ${chunkIndex} for ${assetKey}:`, err);
        void deleteChunkRecords(assetKey, chunkIndex, partCount).catch((deleteErr) => {
            console.warn(`Renegade range cache failed to delete unreadable chunk ${chunkIndex} for ${assetKey}:`, deleteErr);
        });
    };

    const readRequestResult = (request, storeName, key) => {
        try {
            return request.result || null;
        } catch (err) {
            dropUnreadableRecord(storeName, key, err);
            return null;
        }
    };

    const getRecord = async (storeName, key) => {
        const db = await openDb();
        return await new Promise((resolve) => {
            const tx = db.transaction(storeName, 'readonly');
            const request = tx.objectStore(storeName).get(key);
            request.onsuccess = () => resolve(readRequestResult(request, storeName, key));
            request.onerror = (event) => {
                if (event) {
                    event.preventDefault();
                    if (event.stopPropagation) {
                        event.stopPropagation();
                    }
                }
                dropUnreadableRecord(storeName, key, request.error);
                resolve(null);
            };
        });
    };

    const getChunkMeta = async (assetKey, chunkIndex) => {
        const meta = await getRecord(chunkMetaStoreName, chunkMetaKey(assetKey, chunkIndex));
        const partCount = meta && Number.isInteger(meta.partCount) && meta.partCount > 0 ? meta.partCount : 0;
        if (!meta || meta.chunkIndex !== chunkIndex || !Number.isInteger(meta.byteLength) || meta.byteLength <= 0 || partCount <= 0) {
            if (meta) {
                void deleteChunkRecords(assetKey, chunkIndex, partCount).catch((err) => {
                    console.warn(`Renegade range cache failed to delete invalid chunk ${chunkIndex} for ${assetKey}:`, err);
                });
            }
            return null;
        }

        return {
            byteLength: meta.byteLength,
            partCount
        };
    };

    const getChunk = async (assetKey, chunkIndex) => {
        const chunkMeta = await getChunkMeta(assetKey, chunkIndex);
        if (!chunkMeta) {
            return null;
        }

        const db = await openDb();
        return await new Promise((resolve) => {
            const tx = db.transaction(chunkPartStoreName, 'readonly');
            const store = tx.objectStore(chunkPartStoreName);
            const parts = new Array(chunkMeta.partCount);
            let completed = 0;
            let settled = false;
            const settleCacheMiss = () => {
                if (settled) {
                    return;
                }
                settled = true;
                void deleteChunkRecords(assetKey, chunkIndex, chunkMeta.partCount).catch((err) => {
                    console.warn(`Renegade range cache failed to delete unreadable chunk ${chunkIndex} for ${assetKey}:`, err);
                });
                resolve(null);
            };

            for (let partIndex = 0; partIndex < chunkMeta.partCount; ++partIndex) {
                const key = chunkPartKey(assetKey, chunkIndex, partIndex);
                const request = store.get(key);
                request.onsuccess = () => {
                    if (settled) {
                        return;
                    }

                    const record = readRequestResult(request, chunkPartStoreName, key);
                    if (!record || record.chunkIndex !== chunkIndex || record.partIndex !== partIndex) {
                        settleCacheMiss();
                        return;
                    }

                    parts[partIndex] = normalizeBytes(record.bytes);
                    ++completed;
                    if (completed === chunkMeta.partCount) {
                        let totalBytes = 0;
                        for (const partBytes of parts) {
                            totalBytes += partBytes.length;
                        }

                        if (totalBytes !== chunkMeta.byteLength) {
                            settleCacheMiss();
                            return;
                        }

                        const bytes = new Uint8Array(totalBytes);
                        let offset = 0;
                        for (const partBytes of parts) {
                            bytes.set(partBytes, offset);
                            offset += partBytes.length;
                        }

                        settled = true;
                        resolve(bytes);
                    }
                };
                request.onerror = (event) => {
                    if (!settled) {
                        if (event) {
                            event.preventDefault();
                            if (event.stopPropagation) {
                                event.stopPropagation();
                            }
                        }
                        settled = true;
                        dropUnreadableChunk(assetKey, chunkIndex, chunkMeta.partCount, request.error);
                        resolve(null);
                    }
                };
            }
        });
    };

    const getChunks = async (assetKey, firstChunkIndex, chunkCount) => {
        if (chunkCount <= 0) {
            return [];
        }

        const chunks = [];
        for (let i = 0; i < chunkCount; ++i) {
            const chunk = await getChunk(assetKey, firstChunkIndex + i);
            if (!chunk) {
                return null;
            }
            chunks.push(chunk);
        }
        return chunks;
    };

    const putRecords = async (storeName, values) => {
        if (!values.length) {
            return;
        }

        const db = await openDb();
        const tx = db.transaction(storeName, 'readwrite');
        const store = tx.objectStore(storeName);
        for (const value of values) {
            store.put(value);
        }
        await transactionDone(tx);
    };

    const pendingChunkFetches = () => {
        if (!Module.renegadeRangeChunkFetchPromises) {
            Module.renegadeRangeChunkFetchPromises = new Map();
        }
        return Module.renegadeRangeChunkFetchPromises;
    };

    const pendingChunkFetchKey = (assetKey, chunkIndex) => `${assetKey}\nfetch\n${chunkIndex}`;

    Module.renegadeRangeCacheHelpers = {
        assetKey(assetUrl) {
            return assetUrl.href;
        },
        normalizeBytes,
        async getFileSize(assetKey) {
            const record = await getRecord('files', assetKey);
            return record && Number.isFinite(record.size) ? record.size : -1;
        },
        async hasChunk(assetKey, chunkIndex) {
            return !!(await getChunkMeta(assetKey, chunkIndex));
        },
        getChunks,
        getPendingChunkFetch(assetKey, chunkIndex) {
            return pendingChunkFetches().get(pendingChunkFetchKey(assetKey, chunkIndex)) || null;
        },
        startPendingChunkFetch(assetKey, chunkIndex, createPromise) {
            const fetches = pendingChunkFetches();
            const key = pendingChunkFetchKey(assetKey, chunkIndex);
            let promise = fetches.get(key);
            if (!promise) {
                promise = Promise.resolve().then(createPromise);
                fetches.set(key, promise);
                promise.finally(() => {
                    if (fetches.get(key) === promise) {
                        fetches.delete(key);
                    }
                });
            }
            return promise;
        },
        async putFileSize(assetKey, size) {
            if (!Number.isFinite(size) || size < 0) {
                return;
            }
            await putRecords('files', [{ key: assetKey, size }]);
        },
        async putFetchedChunks(assetKey, firstChunkIndex, chunkSize, bytes) {
            const chunkBytes = normalizeBytes(bytes);
            if (!chunkBytes.length) {
                return;
            }

            const db = await openDb();
            const tx = db.transaction([chunkMetaStoreName, chunkPartStoreName], 'readwrite');
            const metaStore = tx.objectStore(chunkMetaStoreName);
            const partStore = tx.objectStore(chunkPartStoreName);
            let cursor = 0;
            let chunkIndex = firstChunkIndex;
            while (cursor < chunkBytes.length) {
                const nextCursor = Math.min(cursor + chunkSize, chunkBytes.length);
                const chunk = chunkBytes.slice(cursor, nextCursor);
                const partCount = Math.ceil(chunk.length / persistedChunkPartSize);
                metaStore.put({
                    key: chunkMetaKey(assetKey, chunkIndex),
                    chunkIndex,
                    byteLength: chunk.length,
                    partCount
                });
                let partOffset = 0;
                let partIndex = 0;
                while (partOffset < chunk.length) {
                    const nextPartOffset = Math.min(partOffset + persistedChunkPartSize, chunk.length);
                    partStore.put({
                        key: chunkPartKey(assetKey, chunkIndex, partIndex),
                        chunkIndex,
                        partIndex,
                        bytes: chunk.slice(partOffset, nextPartOffset)
                    });
                    partOffset = nextPartOffset;
                    ++partIndex;
                }
                cursor = nextCursor;
                ++chunkIndex;
            }

            await transactionDone(tx);
        },
        parseContentRangeSize
    };
});

EM_ASYNC_JS(int, renegade_emscripten_query_range_asset_size_js, (const char * remote_relative_c, const char * default_base_url_c), {
    const remoteRelative = UTF8ToString(remote_relative_c);
    const defaultBaseUrl = UTF8ToString(default_base_url_c);
    const configuredBaseUrl = Module.renegadeAssetBaseUrl || globalThis.RENEGADE_ASSET_BASE_URL || defaultBaseUrl;
    const absoluteBaseUrl = new URL(configuredBaseUrl, globalThis.location.href);
    const assetUrl = new URL(remoteRelative, absoluteBaseUrl);
    const rangeCache = Module.renegadeRangeCacheHelpers;
    if (!rangeCache) {
        console.error(`Renegade range size probe failed for ${assetUrl}: range cache helpers are not initialized`);
        return -1;
    }

    const cacheKey = rangeCache.assetKey(assetUrl);

    try {
        const cachedSize = await rangeCache.getFileSize(cacheKey);
        if (cachedSize >= 0) {
            return cachedSize;
        }

        const response = await fetch(assetUrl, {
            credentials: 'same-origin',
            headers: { 'Range': 'bytes=0-0' }
        });

        if (!response.ok) {
            console.error(`Renegade range size probe failed for ${assetUrl}: ${response.status} ${response.statusText}`);
            return -1;
        }

        if (response.status !== 206) {
            if (response.body) {
                try {
                    await response.body.cancel();
                } catch (err) {
                }
            }
            return -2;
        }

        await response.arrayBuffer();
        const totalSize = rangeCache.parseContentRangeSize(response);
        if (totalSize >= 0) {
            await rangeCache.putFileSize(cacheKey, totalSize);
        }
        return totalSize;
    } catch (err) {
        console.error(`Renegade range size probe failed for ${assetUrl}:`, err);
        return -1;
    }
});

EM_ASYNC_JS(int, renegade_emscripten_load_or_fetch_asset_range_span_js, (const char * remote_relative_c, const char * default_base_url_c, int offset, int length, int chunk_size, unsigned char * dest, int dest_capacity), {
    const remoteRelative = UTF8ToString(remote_relative_c);
    const defaultBaseUrl = UTF8ToString(default_base_url_c);
    const configuredBaseUrl = Module.renegadeAssetBaseUrl || globalThis.RENEGADE_ASSET_BASE_URL || defaultBaseUrl;
    const absoluteBaseUrl = new URL(configuredBaseUrl, globalThis.location.href);
    const assetUrl = new URL(remoteRelative, absoluteBaseUrl);
    const rangeCache = Module.renegadeRangeCacheHelpers;
    if (!rangeCache) {
        console.error(`Renegade range fetch failed for ${assetUrl}: range cache helpers are not initialized`);
        return -1;
    }

    const cacheKey = rangeCache.assetKey(assetUrl);
    const firstChunkIndex = Math.floor(offset / chunk_size);
    const lastChunkIndex = Math.floor((offset + length - 1) / chunk_size);
    const chunkCount = lastChunkIndex - firstChunkIndex + 1;

    const copyChunksToDest = (cachedChunks) => {
        let copied = 0;
        for (let chunkOffset = 0; chunkOffset < cachedChunks.length; ++chunkOffset) {
            const chunkBytes = cachedChunks[chunkOffset];
            const chunkStart = (chunkOffset === 0) ? (offset % chunk_size) : 0;
            if (chunkStart >= chunkBytes.length) {
                continue;
            }

            const available = chunkBytes.length - chunkStart;
            const toCopy = Math.min(available, dest_capacity - copied, length - copied);
            if (toCopy <= 0) {
                break;
            }

            HEAPU8.set(chunkBytes.subarray(chunkStart, chunkStart + toCopy), dest + copied);
            copied += toCopy;
            if (copied >= dest_capacity || copied >= length) {
                break;
            }
        }
        return copied;
    };

    const fetchRequestedRange = async () => {
        const response = await fetch(assetUrl, {
            credentials: 'same-origin',
            headers: { 'Range': `bytes=${offset}-${offset + length - 1}` }
        });

        if (!response.ok) {
            throw new Error(`Renegade range fetch failed for ${assetUrl}: ${response.status} ${response.statusText}`);
        }

        if (response.status !== 206) {
            if (response.body) {
                try {
                    await response.body.cancel();
                } catch (err) {
                }
            }

            const rangeUnsupported = new Error(`HTTP range requests are not available for ${assetUrl}`);
            rangeUnsupported.renegadeRangeUnsupported = true;
            throw rangeUnsupported;
        }

        const bytes = new Uint8Array(await response.arrayBuffer());
        await rangeCache.putFetchedChunks(cacheKey, firstChunkIndex, chunk_size, bytes);
        const totalSize = rangeCache.parseContentRangeSize(response);
        if (totalSize >= 0) {
            await rangeCache.putFileSize(cacheKey, totalSize);
        }
        return bytes;
    };

    try {
        let cachedChunks = await rangeCache.getChunks(cacheKey, firstChunkIndex, chunkCount);
        if (cachedChunks) {
            return copyChunksToDest(cachedChunks);
        }

        if (chunkCount === 1) {
            const pendingFetch = rangeCache.getPendingChunkFetch(cacheKey, firstChunkIndex);
            if (pendingFetch) {
                try {
                    await pendingFetch;
                } catch (err) {
                }
                cachedChunks = await rangeCache.getChunks(cacheKey, firstChunkIndex, chunkCount);
                if (cachedChunks) {
                    return copyChunksToDest(cachedChunks);
                }
            }
        }

        const bytes = (chunkCount === 1)
            ? await rangeCache.startPendingChunkFetch(cacheKey, firstChunkIndex, fetchRequestedRange)
            : await fetchRequestedRange();
        await rangeCache.putFetchedChunks(cacheKey, firstChunkIndex, chunk_size, bytes);
        cachedChunks = await rangeCache.getChunks(cacheKey, firstChunkIndex, chunkCount);
        return cachedChunks ? copyChunksToDest(cachedChunks) : 0;
    } catch (err) {
        if (err && err.renegadeRangeUnsupported) {
            return -2;
        }
        console.error(`Renegade range fetch failed for ${assetUrl}:`, err);
        return -1;
    }
});

EM_JS(void, renegade_emscripten_prefetch_asset_range_span_js, (const char * remote_relative_c, const char * default_base_url_c, int first_chunk_index, int offset, int length, int chunk_size), {
    if (length <= 0 || chunk_size <= 0) {
        return;
    }

    const remoteRelative = UTF8ToString(remote_relative_c);
    const defaultBaseUrl = UTF8ToString(default_base_url_c);
    const configuredBaseUrl = Module.renegadeAssetBaseUrl || globalThis.RENEGADE_ASSET_BASE_URL || defaultBaseUrl;
    const absoluteBaseUrl = new URL(configuredBaseUrl, globalThis.location.href);
    const assetUrl = new URL(remoteRelative, absoluteBaseUrl);
    const rangeCache = Module.renegadeRangeCacheHelpers;
    if (!rangeCache) {
        console.error(`Renegade chunk prefetch failed for ${assetUrl}: range cache helpers are not initialized`);
        return;
    }

    (async () => {
        const cacheKey = rangeCache.assetKey(assetUrl);
        if (rangeCache.getPendingChunkFetch(cacheKey, first_chunk_index)) {
            return;
        }

        if (await rangeCache.hasChunk(cacheKey, first_chunk_index)) {
            return;
        }

        await rangeCache.startPendingChunkFetch(cacheKey, first_chunk_index, async () => {
            const response = await fetch(assetUrl, {
                credentials: 'same-origin',
                headers: { 'Range': `bytes=${offset}-${offset + length - 1}` }
            });

            if (!response.ok) {
                throw new Error(`Renegade chunk prefetch failed for ${assetUrl}: ${response.status} ${response.statusText}`);
            }

            if (response.status !== 206) {
                if (response.body) {
                    try {
                        await response.body.cancel();
                    } catch (err) {
                    }
                }

                const rangeUnsupported = new Error(`HTTP range requests are not available for ${assetUrl}`);
                rangeUnsupported.renegadeRangeUnsupported = true;
                throw rangeUnsupported;
            }

            const bytes = new Uint8Array(await response.arrayBuffer());
            await rangeCache.putFetchedChunks(cacheKey, first_chunk_index, chunk_size, bytes);
            const totalSize = rangeCache.parseContentRangeSize(response);
            if (totalSize >= 0) {
                await rangeCache.putFileSize(cacheKey, totalSize);
            }
            return bytes;
        });
    })().catch((err) => {
        if (err && err.renegadeRangeUnsupported) {
            console.error(`Renegade chunk prefetch skipped for ${assetUrl}: HTTP range requests are not available`);
            return;
        }
        console.error(`Renegade chunk prefetch failed for ${assetUrl}:`, err);
    });
});
#else
static int renegade_emscripten_init_cache_js()
{
    return 0;
}

static int renegade_emscripten_sync_cache_js()
{
    return 0;
}

static int renegade_emscripten_fetch_asset_js(const char *, const char *, const char *)
{
    return 0;
}

static void renegade_emscripten_init_range_cache_helpers_js()
{
}

static int renegade_emscripten_query_range_asset_size_js(const char *, const char *)
{
    return -1;
}

static int renegade_emscripten_load_or_fetch_asset_range_span_js(const char *, const char *, int, int, int, unsigned char *, int)
{
    return -1;
}

static void renegade_emscripten_prefetch_asset_range_span_js(const char *, const char *, int, int, int, int)
{
}
#endif

bool Initialize_Cache()
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    return false;
#else
    std::scoped_lock lock(Get_Cache_Init_Mutex());
    if (Cache_Initialized()) {
        return true;
    }

    if (!renegade_emscripten_init_cache_js()) {
        SDL_SetError("Failed to initialize the Renegade Emscripten asset cache");
        return false;
    }

    Cache_Initialized() = true;
    return true;
#endif
}

bool Sync_Cache()
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    return false;
#else
    if (!Initialize_Cache()) {
        return false;
    }
    if (!renegade_emscripten_sync_cache_js()) {
        SDL_SetError("Failed to sync the Renegade Emscripten asset cache");
        return false;
    }
    return true;
#endif
}

SDL_IOStream * Open_Synced_File_Stream(SDL_IOStream * backing_stream, const char * mode);

bool Ensure_Remote_Asset_Cached(const std::string & asset_cache_path, const std::string & remote_relative_path)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    return false;
#else
    if (Path_Exists_Raw(asset_cache_path)) {
        return true;
    }
    if (!Initialize_Cache()) {
        return false;
    }

    const std::string parent_path = Get_Parent_Path(asset_cache_path);
    if (!parent_path.empty() && !Ensure_Directory_Tree_Raw(parent_path)) {
        SDL_SetError("Failed to create cache directory '%s'", parent_path.c_str());
        return false;
    }

    if (!renegade_emscripten_fetch_asset_js(asset_cache_path.c_str(), remote_relative_path.c_str(), kRenegadeEmscriptenDefaultAssetBaseUrl)) {
        SDL_SetError("Failed to fetch remote asset '%s'", remote_relative_path.c_str());
        return false;
    }

    return Path_Exists_Raw(asset_cache_path);
#endif
}

std::shared_ptr<RangeFileCache> Get_Range_File_Cache(const std::string & canonical_remote_path)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    return {};
#else
    const std::string cache_key = Fold_Path(canonical_remote_path);
    {
        std::scoped_lock lock(Get_Range_File_Caches_Mutex());
        const auto existing = Get_Range_File_Caches().find(cache_key);
        if (existing != Get_Range_File_Caches().end()) {
            return existing->second;
        }
    }

    renegade_emscripten_init_range_cache_helpers_js();
    const int range_size = renegade_emscripten_query_range_asset_size_js(canonical_remote_path.c_str(), kRenegadeEmscriptenDefaultAssetBaseUrl);
    if (range_size <= 0) {
        if (range_size == -2) {
            SDL_SetError("HTTP range requests are not available for '%s'", canonical_remote_path.c_str());
        } else {
            SDL_SetError("Failed to query the size of remote asset '%s'", canonical_remote_path.c_str());
        }
        return {};
    }

    std::shared_ptr<RangeFileCache> file_cache(new (std::nothrow) RangeFileCache());
    if (!file_cache) {
        SDL_OutOfMemory();
        return {};
    }

    file_cache->remote_relative_path = canonical_remote_path;
    file_cache->size = static_cast<Sint64>(range_size);

    std::scoped_lock lock(Get_Range_File_Caches_Mutex());
    auto & caches = Get_Range_File_Caches();
    const auto inserted = caches.emplace(cache_key, file_cache);
    return inserted.second ? file_cache : inserted.first->second;
#endif
}

bool Has_Range_Chunk_Loaded(const std::shared_ptr<RangeFileCache> & file_cache, Sint64 chunk_index)
{
    if (!file_cache || chunk_index < 0) {
        return false;
    }

    std::scoped_lock lock(file_cache->mutex);
    return file_cache->chunks.find(chunk_index) != file_cache->chunks.end();
}

bool Ensure_Range_Chunk_Span_Loaded(const std::shared_ptr<RangeFileCache> & file_cache, Sint64 first_chunk_index, Sint64 chunk_count)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    return false;
#else
    if (!file_cache || first_chunk_index < 0 || chunk_count <= 0) {
        return false;
    }

    const Sint64 chunk_offset = first_chunk_index * kRenegadeEmscriptenRangeChunkSize;
    if (chunk_offset >= file_cache->size) {
        return false;
    }

    const Sint64 max_span_size = file_cache->size - chunk_offset;
    const Sint64 requested_span_size = std::min(max_span_size, chunk_count * kRenegadeEmscriptenRangeChunkSize);
    std::vector<unsigned char> span_data(static_cast<std::size_t>(requested_span_size));

    renegade_emscripten_init_range_cache_helpers_js();
    const int bytes_fetched = renegade_emscripten_load_or_fetch_asset_range_span_js(file_cache->remote_relative_path.c_str(),
        kRenegadeEmscriptenDefaultAssetBaseUrl,
        static_cast<int>(chunk_offset),
        static_cast<int>(span_data.size()),
        static_cast<int>(kRenegadeEmscriptenRangeChunkSize),
        span_data.data(),
        static_cast<int>(span_data.size()));
    if (bytes_fetched <= 0) {
        if (bytes_fetched == -2) {
            SDL_SetError("HTTP range requests are not available for '%s'", file_cache->remote_relative_path.c_str());
        } else {
            SDL_SetError("Failed to fetch a range from remote asset '%s'", file_cache->remote_relative_path.c_str());
        }
        return false;
    }

    span_data.resize(static_cast<std::size_t>(bytes_fetched));

    std::scoped_lock lock(file_cache->mutex);
    auto & chunks = file_cache->chunks;
    std::size_t offset = 0;
    Sint64 chunk_index = first_chunk_index;
    while (offset < span_data.size()) {
        const std::size_t chunk_size = std::min<std::size_t>(span_data.size() - offset, static_cast<std::size_t>(kRenegadeEmscriptenRangeChunkSize));
        chunks.emplace(chunk_index,
            std::vector<unsigned char>(span_data.begin() + static_cast<std::ptrdiff_t>(offset),
                span_data.begin() + static_cast<std::ptrdiff_t>(offset + chunk_size)));
        offset += chunk_size;
        ++chunk_index;
    }
    return true;
#endif
}

void Prefetch_Range_Chunk(const std::shared_ptr<RangeFileCache> & file_cache, Sint64 chunk_index)
{
#if defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA
    if (!file_cache || chunk_index < 0 || Has_Range_Chunk_Loaded(file_cache, chunk_index)) {
        return;
    }

    const Sint64 chunk_offset = chunk_index * kRenegadeEmscriptenRangeChunkSize;
    if (chunk_offset >= file_cache->size) {
        return;
    }

    const Sint64 chunk_size = std::min(file_cache->size - chunk_offset, kRenegadeEmscriptenRangeChunkSize);
    renegade_emscripten_init_range_cache_helpers_js();
    renegade_emscripten_prefetch_asset_range_span_js(file_cache->remote_relative_path.c_str(),
        kRenegadeEmscriptenDefaultAssetBaseUrl,
        static_cast<int>(chunk_index),
        static_cast<int>(chunk_offset),
        static_cast<int>(chunk_size),
        static_cast<int>(kRenegadeEmscriptenRangeChunkSize));
#else
    (void)file_cache;
    (void)chunk_index;
#endif
}

Sint64 SDLCALL Range_Stream_Size(void * userdata)
{
    const auto * stream = static_cast<RangeStreamState *>(userdata);
    return (stream && stream->file) ? stream->file->size : -1;
}

Sint64 SDLCALL Range_Stream_Seek(void * userdata, Sint64 offset, SDL_IOWhence whence)
{
    auto * stream = static_cast<RangeStreamState *>(userdata);
    if (!stream || !stream->file) {
        SDL_SetError("Invalid Renegade range stream");
        return -1;
    }

    Sint64 base = 0;
    switch (whence) {
        case SDL_IO_SEEK_SET:
            base = 0;
            break;
        case SDL_IO_SEEK_CUR:
            base = stream->position;
            break;
        case SDL_IO_SEEK_END:
            base = stream->file->size;
            break;
        default:
            SDL_SetError("Invalid Renegade range seek mode");
            return -1;
    }

    if ((offset < 0 && base < -offset) || (offset > 0 && base > LLONG_MAX - offset)) {
        SDL_SetError("Renegade range seek overflow");
        return -1;
    }

    const Sint64 new_position = base + offset;
    if (new_position < 0) {
        SDL_SetError("Renegade range seek before start of file");
        return -1;
    }

    stream->position = new_position;
    return stream->position;
}

size_t SDLCALL Range_Stream_Read(void * userdata, void * ptr, size_t size, SDL_IOStatus * status)
{
    auto * stream = static_cast<RangeStreamState *>(userdata);
    if (!stream || !stream->file || !ptr) {
        if (status) {
            *status = SDL_IO_STATUS_ERROR;
        }
        SDL_SetError("Invalid Renegade range stream read");
        return 0;
    }

    if (stream->position >= stream->file->size) {
        if (status) {
            *status = SDL_IO_STATUS_EOF;
        }
        return 0;
    }

    const size_t requested = static_cast<size_t>(std::min<Sint64>(static_cast<Sint64>(size), stream->file->size - stream->position));
    unsigned char * output = static_cast<unsigned char *>(ptr);
    size_t copied = 0;

    while (copied < requested) {
        const Sint64 absolute_offset = stream->position + static_cast<Sint64>(copied);
        const Sint64 chunk_index = absolute_offset / kRenegadeEmscriptenRangeChunkSize;
        if (!Has_Range_Chunk_Loaded(stream->file, chunk_index)) {
            if (!Ensure_Range_Chunk_Span_Loaded(stream->file, chunk_index, 1)) {
                if (status) {
                    *status = SDL_IO_STATUS_ERROR;
                }
                return copied;
            }
        }

        size_t to_copy = 0;
        {
            std::scoped_lock lock(stream->file->mutex);
            const auto chunk = stream->file->chunks.find(chunk_index);
            if (chunk == stream->file->chunks.end()) {
                if (status) {
                    *status = SDL_IO_STATUS_ERROR;
                }
                SDL_SetError("Missing cached Renegade range chunk");
                return copied;
            }

            const size_t chunk_offset = static_cast<size_t>(absolute_offset % kRenegadeEmscriptenRangeChunkSize);
            if (chunk_offset >= chunk->second.size()) {
                break;
            }

            const size_t available = chunk->second.size() - chunk_offset;
            to_copy = std::min(available, requested - copied);
            std::memcpy(output + copied, chunk->second.data() + chunk_offset, to_copy);
        }

        Prefetch_Range_Chunk(stream->file, chunk_index + 1);
        copied += to_copy;
    }

    stream->position += static_cast<Sint64>(copied);
    if (status) {
        *status = (copied < requested && stream->position >= stream->file->size) ? SDL_IO_STATUS_EOF : SDL_IO_STATUS_READY;
    }
    return copied;
}

size_t SDLCALL Range_Stream_Write(void *, const void *, size_t, SDL_IOStatus * status)
{
    if (status) {
        *status = SDL_IO_STATUS_READONLY;
    }
    SDL_SetError("Renegade range streams are read-only");
    return 0;
}

bool SDLCALL Range_Stream_Flush(void *, SDL_IOStatus * status)
{
    if (status) {
        *status = SDL_IO_STATUS_READY;
    }
    return true;
}

bool SDLCALL Range_Stream_Close(void * userdata)
{
    delete static_cast<RangeStreamState *>(userdata);
    return true;
}

const SDL_IOStreamInterface & Range_Stream_Interface()
{
    static const SDL_IOStreamInterface interface = []() {
        SDL_IOStreamInterface iface;
        SDL_INIT_INTERFACE(&iface);
        iface.size = Range_Stream_Size;
        iface.seek = Range_Stream_Seek;
        iface.read = Range_Stream_Read;
        iface.write = Range_Stream_Write;
        iface.flush = Range_Stream_Flush;
        iface.close = Range_Stream_Close;
        return iface;
    }();
    return interface;
}

SDL_IOStream * Open_Range_Stream(const std::string & canonical_remote_path)
{
    const std::shared_ptr<RangeFileCache> file_cache = Get_Range_File_Cache(canonical_remote_path);
    if (!file_cache) {
        return nullptr;
    }

    RangeStreamState * stream_state = new (std::nothrow) RangeStreamState();
    if (!stream_state) {
        SDL_OutOfMemory();
        return nullptr;
    }

    stream_state->file = file_cache;
    SDL_IOStream * stream = SDL_OpenIO(&Range_Stream_Interface(), stream_state);
    if (!stream) {
        delete stream_state;
        return nullptr;
    }

    return stream;
}

Sint64 SDLCALL Synced_Stream_Size(void * userdata)
{
    const auto * stream = static_cast<const SyncedFileStreamState *>(userdata);
    if (!stream || !stream->stream) {
        SDL_SetError("Invalid Renegade synced stream");
        return -1;
    }
    return SDL_GetIOSize(stream->stream);
}

Sint64 SDLCALL Synced_Stream_Seek(void * userdata, Sint64 offset, SDL_IOWhence whence)
{
    auto * stream = static_cast<SyncedFileStreamState *>(userdata);
    if (!stream || !stream->stream) {
        SDL_SetError("Invalid Renegade synced stream");
        return -1;
    }
    return SDL_SeekIO(stream->stream, offset, whence);
}

size_t SDLCALL Synced_Stream_Read(void * userdata, void * ptr, size_t size, SDL_IOStatus * status)
{
    auto * stream = static_cast<SyncedFileStreamState *>(userdata);
    if (!stream || !stream->stream) {
        if (status) {
            *status = SDL_IO_STATUS_ERROR;
        }
        SDL_SetError("Invalid Renegade synced stream");
        return 0;
    }
    return SDL_ReadIO(stream->stream, ptr, size);
}

size_t SDLCALL Synced_Stream_Write(void * userdata, const void * ptr, size_t size, SDL_IOStatus * status)
{
    auto * stream = static_cast<SyncedFileStreamState *>(userdata);
    if (!stream || !stream->stream) {
        if (status) {
            *status = SDL_IO_STATUS_ERROR;
        }
        SDL_SetError("Invalid Renegade synced stream");
        return 0;
    }

    const size_t written = SDL_WriteIO(stream->stream, ptr, size);
    if (written > 0) {
        stream->dirty = true;
    }
    return written;
}

bool SDLCALL Synced_Stream_Flush(void * userdata, SDL_IOStatus * status)
{
    auto * stream = static_cast<SyncedFileStreamState *>(userdata);
    if (!stream || !stream->stream) {
        if (status) {
            *status = SDL_IO_STATUS_ERROR;
        }
        SDL_SetError("Invalid Renegade synced stream");
        return false;
    }

    if (!SDL_FlushIO(stream->stream)) {
        if (status) {
            *status = SDL_IO_STATUS_ERROR;
        }
        return false;
    }

    if (stream->dirty && !Sync_Cache()) {
        if (status) {
            *status = SDL_IO_STATUS_ERROR;
        }
        return false;
    }

    if (status) {
        *status = SDL_IO_STATUS_READY;
    }
    stream->dirty = false;
    return true;
}

bool SDLCALL Synced_Stream_Close(void * userdata)
{
    std::unique_ptr<SyncedFileStreamState> stream(static_cast<SyncedFileStreamState *>(userdata));
    if (!stream || !stream->stream) {
        SDL_SetError("Invalid Renegade synced stream");
        return false;
    }

    bool success = SDL_FlushIO(stream->stream);
    if (success && stream->dirty) {
        success = Sync_Cache();
    }
    if (!SDL_CloseIO(stream->stream)) {
        success = false;
    }
    return success;
}

const SDL_IOStreamInterface & Synced_Stream_Interface()
{
    static const SDL_IOStreamInterface interface = []() {
        SDL_IOStreamInterface iface;
        SDL_INIT_INTERFACE(&iface);
        iface.size = Synced_Stream_Size;
        iface.seek = Synced_Stream_Seek;
        iface.read = Synced_Stream_Read;
        iface.write = Synced_Stream_Write;
        iface.flush = Synced_Stream_Flush;
        iface.close = Synced_Stream_Close;
        return iface;
    }();
    return interface;
}

SDL_IOStream * Open_Synced_File_Stream(SDL_IOStream * backing_stream, const char * mode)
{
    SyncedFileStreamState * stream_state = new (std::nothrow) SyncedFileStreamState();
    if (!stream_state) {
        SDL_OutOfMemory();
        SDL_CloseIO(backing_stream);
        return nullptr;
    }

    stream_state->stream = backing_stream;
    stream_state->dirty = mode != nullptr && (std::strchr(mode, 'w') != nullptr || std::strchr(mode, 'a') != nullptr);

    SDL_IOStream * wrapped_stream = SDL_OpenIO(&Synced_Stream_Interface(), stream_state);
    if (!wrapped_stream) {
        SDL_CloseIO(backing_stream);
        delete stream_state;
        return nullptr;
    }
    return wrapped_stream;
}

bool Is_Virtual_Path(const std::string & normalized_path)
{
    SDL_PathInfo ignored = {};
    return renegade_emscripten_assets::Get_Synthetic_Path_Info(normalized_path, &ignored);
}

} // namespace

namespace renegade_emscripten_assets {

bool Get_Synthetic_Path_Info(const std::string & normalized_path, SDL_PathInfo * info)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    (void)normalized_path;
    (void)info;
    return false;
#else
    std::string canonical_request_path;
    std::string canonical_remote_path;
    std::string asset_cache_path;
    std::string user_cache_path;
    const bool has_remote_asset = Resolve_Manifest_Backings(normalized_path,
        canonical_request_path,
        &canonical_remote_path,
        &asset_cache_path,
        &user_cache_path);

    if (!canonical_request_path.empty()) {
        const bool is_local_user_data = Is_Local_User_Data_Request_Path(canonical_request_path);
        const bool allow_seed_fetch = Should_Seed_Local_User_Data_Request_Path(canonical_request_path);

        if (!user_cache_path.empty()) {
            if (Get_Path_Info_Raw(user_cache_path, info)) {
                return true;
            }
        }

        if (is_local_user_data && !allow_seed_fetch) {
            return false;
        }

        if (has_remote_asset && !asset_cache_path.empty() && Get_Path_Info_Raw(asset_cache_path, info)) {
            return true;
        }

        if (has_remote_asset) {
            if (info) {
                std::memset(info, 0, sizeof(*info));
                info->type = SDL_PATHTYPE_FILE;
            }
            return true;
        }

        if (Load_Manifest() && Manifest_Has_Directory(canonical_request_path)) {
            if (info) {
                std::memset(info, 0, sizeof(*info));
                info->type = SDL_PATHTYPE_DIRECTORY;
            }
            return true;
        }
    }

    return false;
#endif
}

void Append_Synthetic_Directory_Entries(const std::string & normalized_directory, std::vector<std::string> & entries)
{
#if defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA
    const std::string canonical_directory = Canonicalize_Request_Path(normalized_directory);
    const std::string directory_key = Directory_Key(canonical_directory);

    std::unordered_set<std::string> seen;
    seen.reserve(entries.size());
    for (const std::string & entry : entries) {
        seen.insert(Fold_Path(entry));
    }

    const std::string user_cache_directory = User_Cache_Path(canonical_directory);
    std::vector<std::string> user_entries;
    struct DirectoryEnumerationContext {
        std::vector<std::string> * entries;
    } context = { &user_entries };
    auto collect_directory_entry = [](void * userdata, const char *, const char * fname) -> SDL_EnumerationResult {
        auto * ctx = static_cast<DirectoryEnumerationContext *>(userdata);
        if (!ctx || !ctx->entries || !fname || !*fname) {
            return SDL_ENUM_FAILURE;
        }
        ctx->entries->emplace_back(fname);
        return SDL_ENUM_CONTINUE;
    };
    if (SDL_EnumerateDirectory(user_cache_directory.c_str(), collect_directory_entry, &context)) {
        for (const std::string & entry : user_entries) {
            if (seen.insert(Fold_Path(entry)).second) {
                entries.push_back(entry);
            }
        }
    }

    if (Load_Manifest()) {
        const auto manifest_entries = Get_Manifest_State().directory_entries.find(directory_key);
        if (manifest_entries != Get_Manifest_State().directory_entries.end()) {
            for (const std::string & entry : manifest_entries->second) {
                if (seen.insert(Fold_Path(entry)).second) {
                    entries.push_back(entry);
                }
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](const std::string & lhs, const std::string & rhs) {
        return Fold_Path(lhs) < Fold_Path(rhs);
    });
#else
    (void)normalized_directory;
    (void)entries;
#endif
}

SDL_IOStream * Open_File(const std::string & normalized_path, const char * mode)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    (void)normalized_path;
    (void)mode;
    return nullptr;
#else
    if (!mode || normalized_path.empty()) {
        return nullptr;
    }

    std::string canonical_request_path;
    std::string canonical_remote_path;
    std::string asset_cache_path;
    std::string user_cache_path;
    const bool has_remote_asset = Resolve_Manifest_Backings(normalized_path,
        canonical_request_path,
        &canonical_remote_path,
        &asset_cache_path,
        &user_cache_path);

    if (canonical_request_path.empty()) {
        return nullptr;
    }

    const bool is_local_user_data = Is_Local_User_Data_Request_Path(canonical_request_path);
    const bool allow_seed_fetch = Should_Seed_Local_User_Data_Request_Path(canonical_request_path);
    const bool allow_fetch = std::strchr(mode, 'r') != nullptr || std::strchr(mode, '+') != nullptr;

    if (is_local_user_data) {
        if (!Initialize_Cache()) {
            return nullptr;
        }

        const std::string parent_path = Get_Parent_Path(user_cache_path);
        if (!parent_path.empty() && !Ensure_Directory_Tree_Raw(parent_path)) {
            SDL_SetError("Failed to create writable cache directory '%s'", parent_path.c_str());
            return nullptr;
        }

        if (!Path_Exists_Raw(user_cache_path) && has_remote_asset && allow_seed_fetch && Open_Mode_Needs_Seeded_Content(mode)) {
            if (!Ensure_Remote_Asset_Cached(user_cache_path, canonical_remote_path)) {
                return nullptr;
            }
        }

        SDL_IOStream * stream = SDL_IOFromFile(user_cache_path.c_str(), mode);
        if (!stream) {
            return nullptr;
        }
        return Open_Synced_File_Stream(stream, mode);
    }

    if (!has_remote_asset) {
        return nullptr;
    }

    if (allow_fetch && Is_Read_Only_Open_Mode(mode) && !Path_Exists_Raw(asset_cache_path)) {
        SDL_IOStream * range_stream = Open_Range_Stream(canonical_remote_path);
        if (range_stream != nullptr) {
            return range_stream;
        }
    }

    if (allow_fetch && !Path_Exists_Raw(asset_cache_path) && !Ensure_Remote_Asset_Cached(asset_cache_path, canonical_remote_path)) {
        return nullptr;
    }

    if (!Path_Exists_Raw(asset_cache_path)) {
        return nullptr;
    }

    return SDL_IOFromFile(asset_cache_path.c_str(), mode);
#endif
}

bool Create_Directory(const std::string & normalized_path)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    (void)normalized_path;
    return false;
#else
    const std::string canonical_request_path = Canonicalize_Request_Path(normalized_path);
    if (canonical_request_path.empty()) {
        return false;
    }

    if (!Is_Local_User_Data_Request_Path(canonical_request_path)) {
        return false;
    }

    SDL_PathInfo info = {};
    if (Get_Synthetic_Path_Info(normalized_path, &info)) {
        SDL_SetError("Path already exists");
        return false;
    }

    if (!Initialize_Cache()) {
        return false;
    }

    return Ensure_Directory_Tree_Raw(User_Cache_Path(canonical_request_path));
#endif
}

bool Remove_Path(const std::string & normalized_path)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    (void)normalized_path;
    return false;
#else
    const std::string canonical_request_path = Canonicalize_Request_Path(normalized_path);
    if (canonical_request_path.empty()) {
        return false;
    }

    if (!Is_Local_User_Data_Request_Path(canonical_request_path)) {
        return false;
    }

    const std::string user_cache_path = User_Cache_Path(canonical_request_path);
    if (!Path_Exists_Raw(user_cache_path)) {
        return false;
    }

    if (!SDL_RemovePath(user_cache_path.c_str())) {
        return false;
    }
    return Sync_Cache();
#endif
}

bool Rename_Path(const std::string & old_normalized_path, const std::string & new_normalized_path)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    (void)old_normalized_path;
    (void)new_normalized_path;
    return false;
#else
    const std::string old_canonical_path = Canonicalize_Request_Path(old_normalized_path);
    const std::string new_canonical_path = Canonicalize_Request_Path(new_normalized_path);
    if (old_canonical_path.empty() || new_canonical_path.empty()) {
        return false;
    }

    if (!Is_Local_User_Data_Request_Path(old_canonical_path) || !Is_Local_User_Data_Request_Path(new_canonical_path)) {
        return false;
    }

    const std::string old_user_cache_path = User_Cache_Path(old_canonical_path);
    if (!Path_Exists_Raw(old_user_cache_path)) {
        return false;
    }

    const std::string new_user_cache_path = User_Cache_Path(new_canonical_path);
    const std::string new_parent_path = Get_Parent_Path(new_user_cache_path);
    if (!new_parent_path.empty() && !Ensure_Directory_Tree_Raw(new_parent_path)) {
        return false;
    }

    if (!SDL_RenamePath(old_user_cache_path.c_str(), new_user_cache_path.c_str())) {
        return false;
    }
    return Sync_Cache();
#endif
}

bool Get_File_Attributes(const std::string & normalized_path, uint32_t & attributes)
{
#if !(defined(__EMSCRIPTEN__) && RENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA)
    (void)normalized_path;
    (void)attributes;
    return false;
#else
    SDL_PathInfo info = {};
    if (!Get_Synthetic_Path_Info(normalized_path, &info)) {
        return false;
    }

    attributes = 0;
    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        attributes |= 0x00000010u;
        return true;
    }

    std::string canonical_request_path;
    std::string canonical_remote_path;
    std::string asset_cache_path;
    std::string user_cache_path;
    const bool has_remote_asset = Resolve_Manifest_Backings(normalized_path,
        canonical_request_path,
        &canonical_remote_path,
        &asset_cache_path,
        &user_cache_path);

    if (Is_Local_User_Data_Request_Path(canonical_request_path) && !user_cache_path.empty() && Path_Exists_Raw(user_cache_path)) {
        SDL_IOStream * stream = SDL_IOFromFile(user_cache_path.c_str(), "rb+");
        if (!stream) {
            attributes |= 0x00000001u;
        } else {
            SDL_CloseIO(stream);
        }
        return true;
    }

    if (!asset_cache_path.empty() && Path_Exists_Raw(asset_cache_path)) {
        SDL_IOStream * stream = SDL_IOFromFile(asset_cache_path.c_str(), "rb+");
        if (!stream) {
            attributes |= 0x00000001u;
        } else {
            SDL_CloseIO(stream);
        }
        return true;
    }

    if (has_remote_asset) {
        attributes |= 0x00000001u;
        return true;
    }

    return true;
#endif
}

} // namespace renegade_emscripten_assets

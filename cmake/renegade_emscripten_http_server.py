#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import shutil
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


class RangeRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        self.send_header("Accept-Ranges", "bytes")
        super().end_headers()

    def send_head(self):
        path = self.translate_path(self.path)
        if os.path.isdir(path):
            return super().send_head()

        try:
            file_handle = open(path, "rb")
        except OSError:
            self.send_error(HTTPStatus.NOT_FOUND, "File not found")
            return None

        try:
            size = os.fstat(file_handle.fileno()).st_size
            range_start, range_end = self._parse_range_header(size)
            if range_start is None:
                self.range = None
                self.send_response(HTTPStatus.OK)
                self.send_header("Content-Type", self.guess_type(path))
                self.send_header("Content-Length", str(size))
            else:
                self.range = (range_start, range_end)
                self.send_response(HTTPStatus.PARTIAL_CONTENT)
                self.send_header("Content-Type", self.guess_type(path))
                self.send_header("Content-Length", str(range_end - range_start + 1))
                self.send_header("Content-Range", f"bytes {range_start}-{range_end}/{size}")
                file_handle.seek(range_start)

            self.send_header("Last-Modified", self.date_time_string(os.path.getmtime(path)))
            self.end_headers()
            return file_handle
        except Exception:
            file_handle.close()
            raise

    def copyfile(self, source, outputfile) -> None:
        range_value = getattr(self, "range", None)
        if range_value is None:
            shutil.copyfileobj(source, outputfile)
            return

        remaining = range_value[1] - range_value[0] + 1
        while remaining > 0:
            chunk = source.read(min(64 * 1024, remaining))
            if not chunk:
                break
            outputfile.write(chunk)
            remaining -= len(chunk)

    def _parse_range_header(self, size: int) -> tuple[int | None, int | None]:
        header_value = self.headers.get("Range")
        if not header_value:
            return (None, None)

        unit, _, range_spec = header_value.partition("=")
        if unit.strip().lower() != "bytes" or not range_spec:
            self._send_range_not_satisfiable(size)
            raise ValueError("Unsupported range request")

        if "," in range_spec:
            self._send_range_not_satisfiable(size)
            raise ValueError("Multiple ranges are not supported")

        start_text, _, end_text = range_spec.strip().partition("-")
        try:
            if start_text == "":
                suffix_length = int(end_text)
                if suffix_length <= 0:
                    raise ValueError("Suffix length must be positive")
                suffix_length = min(suffix_length, size)
                start = size - suffix_length
                end = size - 1
            else:
                start = int(start_text)
                end = size - 1 if end_text == "" else int(end_text)
                if start < 0 or start >= size:
                    raise ValueError("Range start is outside file bounds")
                end = min(end, size - 1)
                if end < start:
                    raise ValueError("Range end precedes range start")
        except ValueError:
            self._send_range_not_satisfiable(size)
            raise

        return (start, end)

    def _send_range_not_satisfiable(self, size: int) -> None:
        self.send_response(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE)
        self.send_header("Content-Range", f"bytes */{size}")
        self.send_header("Content-Length", "0")
        self.end_headers()


def main() -> int:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("port", nargs="?", type=int, default=8080)
    parser.add_argument(
        "-d",
        "--directory",
        default=os.getcwd(),
        help="Directory to serve",
    )
    parser.add_argument(
        "--bind",
        default="127.0.0.1",
        help="Address to bind to",
    )
    args = parser.parse_args()

    class Server(ThreadingHTTPServer):
        daemon_threads = True

    handler = lambda *handler_args, **handler_kwargs: RangeRequestHandler(
        *handler_args,
        directory=args.directory,
        **handler_kwargs,
    )

    with Server((args.bind, args.port), handler) as httpd:
        print(f"Serving {args.directory} on http://{args.bind}:{args.port}/")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nStopping server.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

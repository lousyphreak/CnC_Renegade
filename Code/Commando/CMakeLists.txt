set(COMMANDO_SHARED_SOURCES
    apppacketstats.cpp
    bioevent.cpp
    campaign.cpp
    changeteamevent.cpp
    chatshre.cpp
    clientbboevent.cpp
    clientfps.cpp
    clientgoodbyeevent.cpp
    commandosaveload.cpp
    comnetrcvinst.cpp
    consolecommandevent.cpp
    csconsolecommandevent.cpp
    cshint.cpp
    cspingrequestevent.cpp
    cstextobj.cpp
    datasafe.cpp
    devoptions.cpp
    donateevent.cpp
    floodprotectionmgr.cpp
    gamedata.cpp
    gamedataupdateevent.cpp
    gameinitmgr.cpp
    gamemode.cpp
    gamesideservercontrol.cpp
    gdcnc.cpp
    gdsingleplayer.cpp
    gdskirmish.cpp
    god.cpp
    godmodeevent.cpp
    langmode.cpp
    level.cpp
    loadingevent.cpp
    mainloop.cpp
    messages.cpp
    modpackage.cpp
    modpackagemgr.cpp
    moneyevent.cpp
    nataddr.cpp
    natsock.cpp
    netevent.cpp
    nethandler.cpp
    netinterface.cpp
    nicenum.cpp
    pkthandlers.cpp
    player.cpp
    playerkill.cpp
    playermanager.cpp
    priority.cpp
    purchaserequestevent.cpp
    purchaseresponseevent.cpp
    regbool.cpp
    regfloat.cpp
    regint.cpp
    regstring.cpp
    requestkillevent.cpp
    scoreevent.cpp
    scpingresponseevent.cpp
    singletoninstancekeeper.cpp
    skinpackage.cpp
    skinpackagemgr.cpp
    slavemaster.cpp
    suicideevent.cpp
    svrgoodbyeevent.cpp
    systemsettings.cpp
    team.cpp
    teammanager.cpp
    useroptions.cpp
    vendor.cpp
    vipmodeevent.cpp
    warpevent.cpp
    winevent.cpp
    commando_bootstrap_bridge.cpp
    commando_service_stubs.cpp
)

set(COMMANDO_EXECUTABLE_SOURCES
    commando_sdl_main.cpp
)

set(COMMANDO_CLIENT_MENU_SOURCES
    dlgcharacteroptions.cpp
    dlgcheatoptions.cpp
    dlgconfigaudiotab.cpp
    dlgconfigperformancetab.cpp
    dlgconfigvideotab.cpp
    dlgcontroltabs.cpp
    dlgcontrols.cpp
    dlgcontrolsaveload.cpp
    dlgcontrolslisttab.cpp
    dlgcredits.cpp
    dlgmovieoptions.cpp
    dlgpreviewoptions.cpp
    dlgtechoptions.cpp
)

set(COMMANDO_SOURCES ${COMMANDO_SHARED_SOURCES})

list(APPEND COMMANDO_SOURCES
    AnnounceEvent.cpp
    bandwidth.cpp
    bandwidthgraph.cpp
    buildnum.cpp
    cnetwork.cpp
    combatgmode.cpp
    console.cpp
    consolefunction.cpp
    diagnostics.cpp
    commando_dedicated_ui_stubs.cpp
    dlgwebpage_stub.cpp
    dlgcncwinscreen.cpp
    dlgmainmenu.cpp
    evictionevent.cpp
    gamemenu.cpp
    gameoptionsevent.cpp
    init.cpp
    inputconfig.cpp
    inputconfigmgr.cpp
    lanchat.cpp
    mainmenutransition.cpp
    menu_dialog_subset.cpp
    movie.cpp
    mpsettingsmgr.cpp
    multihud.cpp
    overlay.cpp
    renegadedialogmgr_dedicated.cpp
    renegadecheatmgr.cpp
    sbbomanager.cpp
    sctextobj.cpp
    scorescreen.cpp
    ServerSettings.cpp
    serverfps.cpp
    shutdown.cpp
)

list(APPEND COMMANDO_SOURCES
    ${COMMANDO_CLIENT_MENU_SOURCES}
)

if(WIN32)
    list(APPEND COMMANDO_SOURCES ConsoleMode.cpp)
endif()

list(REMOVE_ITEM COMMANDO_SOURCES
    cdverify.cpp
    slavemaster.cpp
)

set(COMMANDO_DEDICATED_SOURCES ${COMMANDO_SOURCES})

list(APPEND COMMANDO_DEDICATED_SOURCES
    AnnounceEvent.cpp
    bandwidth.cpp
    bandwidthgraph.cpp
    buildnum.cpp
    campaign.cpp
    cnetwork.cpp
    combatgmode.cpp
    console.cpp
    consolefunction.cpp
    diagnostics.cpp
    dlgcncwinscreen.cpp
    evictionevent.cpp
    floodprotectionmgr.cpp
    gamemenu.cpp
    gameoptionsevent.cpp
    gameinitmgr.cpp
    gamesideservercontrol.cpp
    gdcnc.cpp
    init.cpp
    lanchat.cpp
    mainloop.cpp
    movie.cpp
    mpsettingsmgr.cpp
    multihud.cpp
    overlay.cpp
    radiocommanddisplay.cpp
    renegadedialogmgr_dedicated.cpp
    commando_dedicated_ui_stubs.cpp
    sbbomanager.cpp
    sctextobj.cpp
    scorescreen.cpp
    ServerSettings.cpp
    serverfps.cpp
    shutdown.cpp
    textdisplay.cpp
)

list(REMOVE_ITEM COMMANDO_DEDICATED_SOURCES
    dlgmainmenu.cpp
    menu_dialog_subset.cpp
    langmode.cpp
    gamemenu.cpp
    movie.cpp
    overlay.cpp
    scorescreen.cpp
    textdisplay.cpp
    inputconfig.cpp
    inputconfigmgr.cpp
    lanchat.cpp
    radiocommanddisplay.cpp
    ${COMMANDO_CLIENT_MENU_SOURCES}
)

if(WIN32)
    list(REMOVE_ITEM COMMANDO_DEDICATED_SOURCES ConsoleMode.cpp)
endif()

set(COMMANDO_DEDICATED_EXECUTABLE_SOURCES
    commando_dedicated_bootstrap.cpp
    commando_dedicated_sdl_main.cpp
)

file(GLOB COMMANDO_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(CommandoLib STATIC ${COMMANDO_SOURCES} ${COMMANDO_HEADERS})
renegade_configure_target(CommandoLib)

target_compile_definitions(CommandoLib
    PRIVATE
        GAME_BUILD
        GAME_ONLY
)

set(COMMANDO_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/Code"
    "${PROJECT_SOURCE_DIR}/generated"
)

if(NOT WIN32)
    list(APPEND COMMANDO_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code/compat")
endif()

file(GLOB CODE_TOP_LEVEL_DIRS LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/Code/*")
foreach(code_dir IN LISTS CODE_TOP_LEVEL_DIRS)
    if(IS_DIRECTORY "${code_dir}")
        if(WIN32 AND code_dir STREQUAL "${PROJECT_SOURCE_DIR}/Code/compat")
            continue()
        endif()
        list(APPEND COMMANDO_INCLUDE_DIRS "${code_dir}")
    endif()
endforeach()

target_include_directories(CommandoLib
    PRIVATE
        ${COMMANDO_INCLUDE_DIRS}
)

target_link_libraries(CommandoLib
    PUBLIC
        BinkMovie
        Combat
        SControl
        Scripts
        WWAudio
        WWMath
        ww3d2
        wwbitpack
        wwdebug
        wwnet
        wwphys
        wwsaveload
        wwtranslatedb
        wwui
        wwutil
        wwlib
        SDL3::SDL3
)

if(MSVC)
    target_compile_options(CommandoLib PRIVATE /Zc:strictStrings-)
endif()

if(WIN32)
    target_link_libraries(CommandoLib PUBLIC ws2_32)
endif()

add_library(CommandoDedicatedLib STATIC ${COMMANDO_DEDICATED_SOURCES} ${COMMANDO_HEADERS})
renegade_configure_target(CommandoDedicatedLib)

target_compile_definitions(CommandoDedicatedLib
    PRIVATE
        FREEDEDICATEDSERVER
        GAME_BUILD
        GAME_ONLY
)

target_include_directories(CommandoDedicatedLib
    PRIVATE
        ${COMMANDO_INCLUDE_DIRS}
)

target_link_libraries(CommandoDedicatedLib
    PUBLIC
        BinkMovie
        Combat
        SControl
        Scripts
        WWAudio
        WWMath
        ww3d2
        wwbitpack
        wwdebug
        wwnet
        wwphys
        wwsaveload
        wwtranslatedb
        wwui
        wwutil
        wwlib
        SDL3::SDL3
)

if(MSVC)
    target_compile_options(CommandoDedicatedLib PRIVATE /Zc:strictStrings-)
endif()

if(WIN32)
    target_link_libraries(CommandoDedicatedLib PUBLIC ws2_32)
endif()

add_executable(Commando ${COMMANDO_EXECUTABLE_SOURCES})
renegade_configure_target(Commando)

target_link_libraries(Commando
    PRIVATE
        CommandoLib
        Combat
        CommandoLib
        SDL3::SDL3
)

set_target_properties(Commando PROPERTIES
    OUTPUT_NAME "Renegade"
)

add_executable(CommandoDedicated ${COMMANDO_DEDICATED_EXECUTABLE_SOURCES})
renegade_configure_target(CommandoDedicated)

target_link_libraries(CommandoDedicated
    PRIVATE
        CommandoDedicatedLib
        Combat
        CommandoDedicatedLib
        SDL3::SDL3
)

set_target_properties(CommandoDedicated PROPERTIES
    OUTPUT_NAME "renegade_dedicated"
)

if(TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET Commando POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
            "$<TARGET_FILE_DIR:Commando>"
        VERBATIM
    )

    add_custom_command(TARGET CommandoDedicated POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
            "$<TARGET_FILE_DIR:CommandoDedicated>"
        VERBATIM
    )
endif()

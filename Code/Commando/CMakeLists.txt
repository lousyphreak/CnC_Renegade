set(COMMANDO_SOURCES
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

list(REMOVE_ITEM COMMANDO_SOURCES
    buildnum.cpp
    campaign.cpp
    cdverify.cpp
    evictionevent.cpp
    floodprotectionmgr.cpp
    gameinitmgr.cpp
    gamemode.cpp
    gamesideservercontrol.cpp
    gdcnc.cpp
    init.cpp
    mainloop.cpp
    slavemaster.cpp
    systemsettings.cpp
)

file(GLOB COMMANDO_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(Commando STATIC ${COMMANDO_SOURCES} ${COMMANDO_HEADERS})
renegade_configure_target(Commando)

target_compile_definitions(Commando
    PRIVATE
        GAME_BUILD
        GAME_ONLY
)

set(COMMANDO_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/Code"
    "${PROJECT_SOURCE_DIR}/generated"
    "${PROJECT_SOURCE_DIR}/Code/compat"
)

file(GLOB CODE_TOP_LEVEL_DIRS LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/Code/*")
foreach(code_dir IN LISTS CODE_TOP_LEVEL_DIRS)
    if(IS_DIRECTORY "${code_dir}")
        list(APPEND COMMANDO_INCLUDE_DIRS "${code_dir}")
    endif()
endforeach()

target_include_directories(Commando
    PRIVATE
        ${COMMANDO_INCLUDE_DIRS}
)

target_link_libraries(Commando
    PUBLIC
        BinkMovie
        Combat
        Scripts
        WWAudio
        WWMath
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

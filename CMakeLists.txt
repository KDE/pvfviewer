cmake_minimum_required (VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (pvfViewer)

find_package (ECM REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include (KDEInstallDirs)
include (KDECMakeSettings)
include (KDECompilerSettings NO_POLICY_SCOPE)
include (ECMInstallIcons)
include (FeatureSummary)

kde_enable_exceptions ()

find_package (Qt5 CONFIG REQUIRED
    Core
    PrintSupport
    Widgets
)

find_package (KF5DocTools)

find_package (KF5 REQUIRED COMPONENTS
    I18n
    KIO
    WidgetsAddons
    XmlGui
)

find_package (SharedMimeInfo)
find_package (Poppler REQUIRED COMPONENTS Qt5)

include_directories (BEFORE ${CMAKE_CURENT_BINARY_DIR})

set (pvfViewer_SRCS
    src/Main.cpp
    src/MainWindow.cpp
    src/ViewerTab.cpp
    src/Thumbnail.cpp

    pvfViewer.qrc
)

file (GLOB pvfViewer_UI ${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui)
ki18n_wrap_ui (pvfViewer_SRCS ${pvfViewer_UI})

if (KF5DocTools_FOUND)
    add_subdirectory (doc)
endif (KF5DocTools_FOUND)

ecm_install_icons (ICONS sc-apps-pvfViewer.svgz sc-mimetypes-application-x-pvf.svgz DESTINATION ${KDE_INSTALL_ICONDIR})

add_executable (pvfViewer ${pvfViewer_SRCS})

target_link_libraries (pvfViewer
    Qt5::Core
    Qt5::PrintSupport
    Qt5::Widgets
    KF5::I18n
    KF5::KIOFileWidgets
    KF5::WidgetsAddons
    KF5::XmlGui
    Poppler::Qt5
)

if (SILENCE_DEPRECATED)
    add_definitions( -Wno-deprecated-declarations )
endif (SILENCE_DEPRECATED)

install (TARGETS pvfViewer DESTINATION ${KDE_INSTALL_BINDIR})
install (FILES org.kde.pvfViewer.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install (FILES org.kde.pvfViewer.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

IF (SHAREDMIMEINFO_FOUND)
    install (FILES pvfViewer.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
    update_xdg_mimetypes (${KDE_INSTALL_MIMEDIR})
ENDIF (SHAREDMIMEINFO_FOUND)

feature_summary (WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

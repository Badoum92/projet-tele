cmake_minimum_required( VERSION 2.8 )
add_definitions( ${GTK2_CFLAGS_OTHER} )
project( nuagesDetection )
find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK2 REQUIRED gtk+-2.0)
include_directories( ${GTK2_INCLUDE_DIRS} )
link_directories( ${GTK2_LIBRARY_DIRS} )
#-----------compilation step
add_executable( imaProjet.exe main.c compute.c )
#-----------linking step
target_link_libraries( imaProjet.exe ${GTK2_LIBRARIES} )


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# frontends
add_subdirectory(app)
add_subdirectory(kontact)
add_subdirectory(runner)

# modules
add_subdirectory(akonadi)
add_subdirectory(domain)
add_subdirectory(presentation)
add_subdirectory(utils)
add_subdirectory(widgets)

# migrator
add_subdirectory(migrator)

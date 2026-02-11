cmake_minimum_required(VERSION 3.15)

# Root entry point for IDEs; keep the real project definition in src
# so existing paths that rely on PROJECT_SOURCE_DIR remain unchanged.
add_subdirectory(src)

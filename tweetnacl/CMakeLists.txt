add_compile_options(-Wno-sign-compare)

file(GLOB sources "*.c" "*.h")

add_library(tweetnacl ${sources})

#include <cstdint>
#include <cassert>

// STL bloat
#include <string>
#include <vector>
#include <unordered_map>


// Rust at home ///////////////////////////////////////////////////////////////
#define fun auto
#define var auto
#define let auto const
#define constant static constexpr auto

#define as static_cast

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;

using usize = size_t;

using f32 = float;
using f64 = double;

using String = std::string;

template<typename T>
using List = std::vector<T>;

template<typename K, typename V>
using Dictionary = std::unordered_map<K, V>;

// TODO: Do something more sophisticated
using Path = String;

// defer //////////////////////////////////////////////////////////////////////
template<typename Lambda>
class Deferrable
{
public:
    Deferrable() = delete;
    Deferrable( Lambda  f ): action( f ) {}
    ~Deferrable() { action(); }
private:
    const Lambda action;
};

// Preprocessor magic so __COUNTER__ can be expanded correctly
#define CONCATENATE( l, r )         DO_CONCATENATE( l, r )
#define DO_CONCATENATE( l, r )      DO_CONCATENATE_2( l, r )
#define DO_CONCATENATE_2( l, r )    l##r

#define defer( f ) const auto CONCATENATE( _deferred, __COUNTER__ ) = Deferrable( [&](){ f; } );


enum struct Compiler: u8 { Clang, GCC, MSVC };


struct Version
{
    u8 major = 0;
    u8 minor = 0;
    u8 patch = 1;
};


struct Target
{
    String       name = "UNNAMED"; // Redundant but convenient
    u8           optimization_level = 0;
    List<String> defines = {};
    List<String> compiler_args = {};
    List<Path>   ignored_paths = {};
};


struct Dependency
{
    enum Linkage: u8 { Static, Dynamic };
    struct Location
    {
        enum Type: u8 { Local, Remote } type;
        Path path;
    };

    String       name     = "UNNAMED";
    Linkage      linkage  = Static;
    Location     location = { .type = Location::Local, .path = "" };
    Version      version  = {0,0,1};
    List<String> targets  = { "All" };
};


struct Project
{
    fun build() {};

    enum Type: u8 { Executable, Library };
    struct Args
    {
        String       name                      = "UNNAMED";
        String       description               = "";
        Version      version                   = {0,0,1};
        List<String> authors                   = {};
        Type         type                      = Executable;
        Compiler     compiler                  = Compiler::Clang;
        u8           cpp_version               = 20;
        bool         enable_cpp_extensions     = false;
        bool         generate_compile_commands = true;
        List<String> command_line_arguments    = {};
        String       default_target            = "Debug";
    };

    Project() = default;
    Project( const Args&& args ):
        name(                       args.name ),
        description(                args.description ),
        version(                    args.version ),
        authors(                    args.authors ),
        type(                       args.type ),
        compiler(                   args.compiler ),
        cpp_version(                args.cpp_version ),
        enable_cpp_extensions(      args.enable_cpp_extensions ),
        generate_compile_commands(  args.generate_compile_commands ),
        command_line_arguments(     args.command_line_arguments ),
        default_target(             args.default_target )
    {}

    // TODO: Add some validation
    constexpr fun AddTarget( const Target&& target ) { targets[ target.name ] = target; }
    constexpr fun AddDependency( const Dependency&& dependency ) { dependencies[ dependency.name ] = dependency; }

    String       name = "INVALID";
    String       description;
    Version      version;
    List<String> authors;
    Type         type;
    Compiler     compiler;
    u8           cpp_version;
    bool         enable_cpp_extensions;
    bool         generate_compile_commands;
    List<String> command_line_arguments;
    String       default_target; // `build` and `run` will use this target if none is provided

    Dictionary<String, Target> targets
    {
        { "Debug", {
            .name = "Debug",
            .optimization_level = 0,
            .defines = { "DEBUG", "NDEBUG" },
            .compiler_args = {
                "g",
                "Wall",
                "Werror",
                "pedantic",
                "fsanitize=undefined,address" }}},

        { "Release", {
            .name = "Release",
            .optimization_level = 3,
            .defines = { "RELEASE" },
            .compiler_args = {
                "Wall",
                "Werror",
                "pedantic",
                "fsanitize=undefined" }}}
    };

    Dictionary<String, Dependency> dependencies{};
};


#if !defined( INCLUDE_AS_HEADER )

#include <cstdio>
#include <dlfcn.h>
#include <filesystem>
namespace fs = std::filesystem;


static let new_project_template = String(R"(
#define INCLUDE_AS_HEADER
#include "dedalo.cpp"
#undef INCLUDE_AS_HEADER

extern "C"
void build( Project* project )
{
    assert( project );

    *project = Project({ .name = "##NAME##" });
}
)");

static let new_main_file_template = String(R"(
#include <stdio.h>

int main( int argc, char* argv[] )
{
    printf("Hello World!\n");
}
)");


using BuildCfgFunPtr = void(*)(Project*);
BuildCfgFunPtr build_cfg = nullptr;


fun main( i32 argc, char* argv[] ) -> i32
{
    // Create the build.cpp file if it doesn't exist already
    {
        // TODO: Check the timestamp to determine if it needs to be recompiled
        if( !fs::is_regular_file( "build.cpp" ) )
        {
            var* build_file = fopen( "build.cpp", "w" );
            assert( build_file );
            // TODO: Get the current dir name and use it to replace ##NAME## in the template
            fwrite( new_project_template.c_str(), sizeof(char), new_project_template.length(), build_file );
            fclose( build_file );
        }
    }

    // Create the directory structure
    {
        fs::create_directory( "vendor" );
        fs::create_directory( "tests"  );
        fs::create_directory( "build"  );
        fs::create_directory( "src"    );

        if( !fs::is_regular_file( "src/main.cpp" ) )
        {
            var* main_file = fopen( "src/main.cpp", "w" );
            assert( main_file );
            fwrite( new_main_file_template.c_str(), sizeof(char), new_main_file_template.length(), main_file );
            fclose( main_file );
        }
    }

    // Compile the build.cpp
    {
        // TODO: Check the timestamp to determine if it needs to be compiled to begin with
        // TODO: Dynamically choose the compiler
        system( "clang++ -fPIC -shared -o ./build/build_script.so build.cpp" );
    }

    // Load the build( Project* ) symbol
    {
        var* dll = dlopen( "./build/build_script.so", RTLD_NOW );
        assert( dll );
        build_cfg = (BuildCfgFunPtr) dlsym(dll, "build");
        assert( build_cfg );
        // No need to close the dll
    }

    Project project_cfg;
    build_cfg( &project_cfg );
}

#endif // !INCLUDE_AS_HEADER


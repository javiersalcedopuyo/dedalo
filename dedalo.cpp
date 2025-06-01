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


#define UNIMPLEMENTED() printf( "UNIMPLEMENTED!\n" )


enum [[nodiscard]] ResultCode
{
    UNKNOWN_ERROR       = -1,
    OK                  = 0,
    // CLI errors
    INVALID_ARGUMENT    = EINVAL,
    INVALID_TARGET      = ELAST + 1,
    INVALID_GENERATOR,
    ALREADY_INITIALIZED,
    BUILD_SCRIPT_LOAD_FAILED,

};


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

static let name_tag = String( "##NAME##" );

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


fun init() -> ResultCode
{
    if( fs::is_regular_file( "build.cpp" ) )
    {
        printf( "Project already initialized. Skipping.\n" );
        return ALREADY_INITIALIZED;
    }

    // Create the directory structure
    {
        fs::create_directory( "dependencies" );
        fs::create_directory( "tests"  );
        fs::create_directory( "build"  );
        {
            fs::create_directory( "build/bin"  );
            fs::create_directory( "build/src"  );
            fs::create_directory( "build/dependencies"  );
        }

        fs::create_directory( "src" );
        {
            var* main_file = fopen( "src/main.cpp", "w" );
            assert( main_file );
            fwrite( new_main_file_template.c_str(), sizeof(char), new_main_file_template.length(), main_file );
            fclose( main_file );
        }
    }

    // Init build.cpp
    {
        var* build_file = fopen( "build.cpp", "w" );
        assert( build_file );

        // Replace the project's name with the current folder's
        let current_folder_name = fs::current_path().stem().string();
        var filled_template = new_project_template;
        filled_template.replace(
            filled_template.find( name_tag ),
            name_tag.length(),
            current_folder_name );

        fwrite( filled_template.c_str(), sizeof(char), filled_template.length(), build_file );
        fclose( build_file );
    }
    return OK;
}


fun build( const Project& project ) -> ResultCode
{
    printf( "Starting build of project \"%s\"...\n", project.name.c_str() );
    defer( printf( "...Done.\n" ) );

    UNIMPLEMENTED();

    // TODO: Fetch any remote dependencies and place them in dependencies/
    // TODO: Compile each file in src/ and dependencies/ into .o in their corresponding directories in build/
    // TODO: Link any dynamic libraries into their corresponding .so in build/bin/
    // TODO: Link everything into the executable in build/bin/

    return OK;
}


fun build() -> ResultCode
{
    // Compile the build.cpp
    {
        // TODO: Check the timestamp to determine if it needs to be compiled to begin with
        // TODO: Dynamically choose the compiler
        if( let error_code = system( "clang++ -fPIC -shared -Wall -Werror -O3 -o ./build/build_script.so build.cpp" ) )
        {
            printf( "Build script compilation failed with error %d.\n", error_code );
            return BUILD_SCRIPT_LOAD_FAILED;
        }
    }

    // Load the build( Project* ) symbol
    {
        var* dll = dlopen( "./build/build_script.so", RTLD_NOW );
        if( !dll )
        {
            printf( "ERROR: Couldn't load build script's DLL.\n" );
            return BUILD_SCRIPT_LOAD_FAILED;
        }
        build_cfg = (BuildCfgFunPtr) dlsym(dll, "build");
        if( !build_cfg )
        {
            printf( "ERROR: Couldn't load build function's symbol.\n" );
            return BUILD_SCRIPT_LOAD_FAILED;
        }
        // No need to close the dll, the OS will clean up after us.
    }

    var project = Project();
    build_cfg( &project );
    return build( project );
}


fun run() -> ResultCode
{
    // TODO:
    UNIMPLEMENTED();
    return OK;
}


fun test() -> ResultCode
{
    // TODO:
    UNIMPLEMENTED();
    return OK;
}


fun main( i32 argc, char* argv[] ) -> i32
{
    if( argc < 2 )
    {
        printf( "No command provided.\n" );
        return INVALID_ARGUMENT;
    }

    let cmd = String( argv[1] );
    if( cmd == "init" )
    {
        return init();
    }
    else if( cmd == "build" )
    {
        return build();
    }
    else if( cmd == "run" )
    {
        var build_result = build();
        if( build_result == OK )
        {
            build_result = run();
        }
        return build_result;
    }
    else if( cmd == "test" )
    {
        return test();
    }
    else
    {
        printf( "ERROR: Unrecognized command %s.\n", cmd.c_str() );
        return INVALID_ARGUMENT;
    }
    return OK;
}

#endif // !INCLUDE_AS_HEADER


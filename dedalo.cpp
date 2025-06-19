#include <cstdint>
#include <cassert>

// STL bloat
#include <string>
#include <format>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <optional>


#define ENABLE_LOGS() 1


// Rust at home ///////////////////////////////////////////////////////////////
#define fun auto
#define var auto
#define let auto const
#define constant static constexpr auto

#define as static_cast

namespace fs = std::filesystem;

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
#define fmt std::format

template<typename T>
using Opt = std::optional<T>;
[[ maybe_unused ]] constant None = std::nullopt;

template<typename T>
using List = std::vector<T>;

template<typename K, typename V>
using Dictionary = std::unordered_map<K, V>;

// TODO: Do something more sophisticated
using Path = fs::path;

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


#define println(...) std::cout << fmt(__VA_ARGS__) << std::endl;


 // TODO: Support MSVC macros
#if ENABLE_LOGS()
    #define LOG(...)        println( "💬 INFO [{} @ {} ln{}]: {}",    __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #define WARNING(...)    println( "⚠️ WARNING [{} @ {} ln{}]: {}", __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #define ERROR(...)      println( "⛔️ ERROR [{} @ {} ln{}]: {}",   __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #define UNIMPLEMENTED() println( "🚧 UNIMPLEMENTED [{} @ {} ln{}]", __func__, __FILE__, __LINE__ )
#else // ENABLE_LOGS
    #define INFO( msg... )
    #define WARNING( msg... )
    #define ERROR( msg... )
    #define UNIMPLEMENTED()
#endif // ENABLE_LOGS


#define unreachable() { ERROR("Unreachable point reached!"); abort(); }


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
    COMPILATION_FAILED,
    LINKING_FAILED,
    RUN_COMMAND_FAILED,
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

    fun find_target( const String& name ) -> Target*
    {
        let iter = targets.find( name );
        if( iter == targets.end() )
            return nullptr;

        return &(iter->second);
    }

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


static let include_paths  = String(" -Isrc -Ilibs"); // TODO: Make this an array of paths?
static let bin_output_dir = String( "./build/bin" );
static let obj_output_dir = bin_output_dir + "/obj";


using BuildCfgFunPtr = void(*)(Project*);
BuildCfgFunPtr build_cfg = nullptr;


fun init() -> ResultCode
{
    if( fs::is_regular_file( "build.cpp" ) )
    {
        WARNING( "Project already initialized. Skipping." );
        return ALREADY_INITIALIZED;
    }

    // Create the directory structure
    {
        fs::create_directory( "libs" );
        // TODO: fs::create_directory( "tests"  );
        fs::create_directories( obj_output_dir );
        // TODO: Create a cache and/or other build system files (ninja, make, CMake...)

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


static fun gather_files(
    const Path&       in_path,
    const String&     extension, // TODO: Support multiple extensions
          List<Path>* paths )
{
    assert( paths );

    if( !fs::is_directory( in_path ) )
    {
        WARNING("Path '{}' is not a directory.", in_path.string() );
        return;
    }

    for( let &entry: fs::directory_iterator( in_path ) )
    {
        if( fs::is_directory( entry ) )
            gather_files( entry.path(), extension, paths ); // Recursion, yay!

        if( fs::is_regular_file( entry ) and entry.path().extension() == extension )
        {
            LOG( "Source file found: {}", entry.path().string() );
            paths->push_back( entry.path() );
        }
    }
}


static constexpr fun get_compiler_name( const Compiler compiler ) -> String
{
    switch (compiler)
    {
        case Compiler::Clang: return "clang++";
        case Compiler::GCC:   return "gcc";
        case Compiler::MSVC:  return "CL";
        default: unreachable();
    }
}


static constexpr fun get_flags_from( const Target& target ) -> String
{
    var result = String();
    for( u32 i = 0; i < target.compiler_args.size(); ++i )
    {
        result += " -" + target.compiler_args[i];
    }
    return result;
}


static constexpr fun get_defines_from( const Target& target ) -> String
{
    var result = String();
    for( u32 i = 0; i < target.defines.size(); ++i )
    {
        result += " -D" + target.defines[i];
    }
    return result;
}


static fun compile(
    const Project&    project,
    const Target&     target,
    const List<Path>& cpp_paths ) -> ResultCode
{
    LOG("Compiling...");
    // TODO: Spread the compilation across multiple threads

    let compiler_name  = get_compiler_name( project.compiler );
    let compiler_flags = get_flags_from( target );
    let defines        = get_defines_from( target );

    for( let& source_file: cpp_paths )
    {
        let command = fmt(
            "{} -std=c++{} {} {} -O{} {} -c {} -o {}/{}.o",
            compiler_name,
            project.cpp_version,
            compiler_flags,
            defines,
            target.optimization_level,
            include_paths,
            source_file.string(),
            obj_output_dir,
            source_file.stem().string() );

        // TODO: Accumulate the errors and continue compiling?
        if( let error = system( command.c_str() ) )
        {
            ERROR( "Compilation of file \"{}\" failed with error {}.\n"
                   "Command:\n\t{}",
                      source_file.filename().string(),
                      error,
                      command );
            return COMPILATION_FAILED;
        }
    }
    return OK;
}


static fun link( const Project& project ) -> ResultCode
{
    LOG("Linking...");
    var command = get_compiler_name( project.compiler );

    // TODO: Link libraries
    command += " -fsanitize=address"; // FIXME: This should be done automatically

    var any_obj_files_found = false;
    for( let& entry: fs::directory_iterator( obj_output_dir ) )
    {
        if( !fs::is_regular_file( entry ) or entry.path().extension() != ".o" )
        {
            continue;
        }
        command += fmt( " {}", entry.path().string() );
        any_obj_files_found = true;
    }
    assert( any_obj_files_found );
    command += fmt( " -o {}/{}", bin_output_dir, project.name );

    if( system( command.c_str() ) != OK )
    {
        ERROR( "Failed linking.\nCommand:\n\t{}", command );
        return LINKING_FAILED;
    }
    return OK;
}


static fun needs_recompiling( const Path& source, const Path& binary ) -> bool
{
    assert( fs::exists( source ) );
    return !fs::exists( binary )
        or fs::last_write_time( source ) > fs::last_write_time( binary );
}


static fun compile_config() -> ResultCode
{
    constant so_path  = "./build/build_script.so";
    constant cpp_path = "./build.cpp";

    if( !needs_recompiling( cpp_path, so_path ) )
    {
        return OK;
    }

    LOG( "Compiling build config..." );
    // TODO: Dynamically choose the compiler
    if( let error = system( "clang++ --std=c++20 -fPIC -shared -o ./build/build_script.so build.cpp" ) )
    {
        ERROR( "Build script compilation failed with error {}.", error );
        return BUILD_SCRIPT_LOAD_FAILED;
    }
    return OK;
}


static fun build() -> ResultCode
{
    // Make sure the build directories exist
    fs::create_directories( obj_output_dir );

    if( let error = compile_config() )
    {
        return error;
    }

    LOG( "Loading build config..." );
    {
        var* dll = dlopen( "./build/build_script.so", RTLD_NOW );
        if( !dll )
        {
            ERROR( "Couldn't load build script's DLL." );
            return BUILD_SCRIPT_LOAD_FAILED;
        }
        build_cfg = (BuildCfgFunPtr) dlsym(dll, "build");
        if( !build_cfg )
        {
            ERROR( "Couldn't load build function's symbol." );
            return BUILD_SCRIPT_LOAD_FAILED;
        }
        // No need to close the dll, the OS will clean up after us.
    }

    var project = Project();
    build_cfg( &project );

    if( let* target = project.find_target( project.default_target ) )
    {
        LOG( "Starting build of project \"{}\" for target \"{}\"...", project.name, target->name );

        // Find all the source files
        var cpp_paths = List<Path>{};
        gather_files( "src", ".cpp", &cpp_paths ); // TODO: Add support for source files with different extensions

        // TODO: Fetch any remote dependencies and place them in libs/
        // TODO: Link any dynamic libraries into their corresponding .so in build/bin/

        var result = compile( project, *target, cpp_paths );
        return result != OK
            ? result
            : link( project );
    }
    else
    {
        return INVALID_TARGET;
    }
}


fun run() -> ResultCode
{
    let project_name = fs::current_path().stem().string();
    let command = "./" + bin_output_dir + "/" + project_name;

    return system( command.c_str() ) == OK
        ? OK
        : RUN_COMMAND_FAILED;
}


fun test() -> ResultCode
{
    // TODO:
    UNIMPLEMENTED();
    return OK;
}


fun clean() -> ResultCode
{
    // TODO: Delete the build folder, compile_commands.json, etc
    UNIMPLEMENTED();
    return OK;
}


fun main( i32 argc, char* argv[] ) -> i32
{
    if( argc < 2 )
    {
        ERROR( "No command provided." );
        return INVALID_ARGUMENT;
    }

    let cmd = String( argv[1] );
    if( cmd == "init" )
    {
        return init();
    }
    else if( cmd == "build" )
    {
        // TODO: Provide a specific target
        return build();
    }
    else if( cmd == "run" )
    {
        // TODO: Pass arguments
        var build_result = build();
        if( build_result == OK )
        {
            build_result = run();
        }
        return build_result;
    }
    else if( cmd == "clean" )
    {
        return clean();
    }
    else if( cmd == "test" )
    {
        return test();
    }
    else
    {
        ERROR( "Unrecognized command '{}'.", cmd );
        return INVALID_ARGUMENT;
    }
    return OK;
}

#endif // !INCLUDE_AS_HEADER


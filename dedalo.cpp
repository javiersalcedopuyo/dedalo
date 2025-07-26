#include <cstdint>
#include <cassert>

// STL bloat
#include <string>
#include <format>
#include <iostream>
#include <vector>
#include <filesystem>
#include <optional>


#define ENABLE_LOGS() 1


// Rust at home ///////////////////////////////////////////////////////////////
#define fun auto
#define var auto
#define let auto const
#define constant static constexpr auto

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
    #define LOG(...)                 println( "💬 INFO [{} @ {} ln{}]: {}",    __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #define WARNING(...)             println( "⚠️ WARNING [{} @ {} ln{}]: {}", __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #define ERROR(...)               println( "⛔️ ERROR [{} @ {} ln{}]: {}",   __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #define UNIMPLEMENTED_MSG( ... ) println( "🚧 UNIMPLEMENTED [{} @ {} ln{}]: {}", __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #define UNIMPLEMENTED()          UNIMPLEMENTED_MSG( "" )
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


// TODO: Add more https://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
enum Sanitizer: u8
{
    No_Sanitizers = 0,
    ASan  = 1 << 0,
    UBSan = 1 << 1,
    TSan  = 1 << 2,
};


struct Version
{
    u8 major = 0;
    u8 minor = 0;
    u8 patch = 1;
};


struct Target
{
    String       name               = "UNNAMED"; // Redundant but convenient
    u8           optimization_level = 0;
    u8           sanitizers         = No_Sanitizers;
    List<String> defines            = {};
    List<String> compiler_args      = {};
    List<Path>   ignored_paths      = {};
};


struct Dependency
{
    struct Location
    {
        enum Type: u8 { Local, Remote } type;
        Path path;
    };

    String       name         = "UNNAMED";
    Location     location     = { .type = Location::Local, .path = "" };
    String       linker_flags = "";
    Version      version      = { 0,0,1 };
    List<String> targets      = { "All" };
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

    // TODO: Add more validation
    constexpr fun add_dependency( const Dependency& dependency )
    {
        assert( dependency.name != "UNNAMED" );
        assert( !dependency.name.empty() );

        let iter = std::find_if(
            dependencies.begin(),
            dependencies.end(),
            [ new_name = dependency.name ]( const Dependency& dep )
            {
                return dep.name == new_name;
            });

        if( iter != dependencies.end() )
        {
            WARNING( "Dependency '{}' already exists. Skipping.", dependency.name );
            return;
        }
        dependencies.push_back( dependency );
    }

    constexpr fun find_target( const String& name ) -> Target*
    {
        let iter = std::find_if( targets.begin(), targets.end(), [ name ]( const Target& target )
        {
            return target.name == name;
        });

        return iter == targets.end()
            ? nullptr
            : &(*iter);
    }

    // TODO: Add more validation
    constexpr fun add_target( const Target& target )
    {
        assert( target.name != "UNNAMED" );
        assert( target.name != "All" && "`All` is a reserved target name" );
        assert( !target.name.empty() );

        // We allow overwriting targets so people can add their own "Debug" and "Release"
        if( var* old_target = find_target( target.name ) )
        {
            *old_target = target;
        }
    }

    String       name = "UNNAMED";
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

    List<Target> targets
    {
        {
            .name               = "Debug",
            .optimization_level = 0,
            .sanitizers         = ASan | UBSan,
            .defines            = { "DEBUG" },
            .compiler_args      = {
                "g",
                "Wall",
                "Werror",
                "pedantic" }
        },
        {
            .name               = "Release",
            .optimization_level = 3,
            .sanitizers         = UBSan,
            .defines            = { "RELEASE" },
            .compiler_args      = {
                "Wall",
                "Werror",
                "pedantic" }
        }
    };

    List<Dependency> dependencies{};
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

    // "Debug" and "Release" targets are provided by default.
    // You can override them by creating a new target with the same name.
    // "Debug" is the default target when none is provided to the `run` command.
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


static let include_paths  = String( " -Isrc -Ilib" ); // TODO: Make this an array of paths?
static let dep_output_dir = String( "./build/dep"  );
static let bin_output_dir = String( "./build/bin"  );
static let json_temp_dir  = Path(   "./build/json" );
static let obj_output_dir = bin_output_dir + "/obj";


using BuildCfgFunPtr = void(*)(Project*);
BuildCfgFunPtr build_cfg = nullptr;


template<typename Container, typename T>
constexpr fun contains( const Container& haystack, const T& needle ) -> bool
{
    return std::find( haystack.begin(), haystack.end(), needle ) != haystack.end();
}


fun init() -> ResultCode
{
    if( fs::is_regular_file( "build.cpp" ) )
    {
        WARNING( "Project already initialized. Skipping." );
        return ALREADY_INITIALIZED;
    }

    // Create the directory structure
    {
        fs::create_directory( "lib" );
        // TODO: fs::create_directory( "tests"  );
        fs::create_directories( obj_output_dir );
        // TODO: Create a cache and/or other build system files (ninja, make, CMake...)

        fs::create_directory( "src" );
        {
            var* main_file = fopen( "src/main.cpp", "w" );
            assert( main_file );
            fputs( new_main_file_template.c_str(), main_file );
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

        fputs( filled_template.c_str(), build_file );
        fclose( build_file );
    }
    return OK;
}


static fun gather_files(
    const Path&         in_path,
    const List<String>& extensions,
    const List<Path>&   excluded_paths,
          List<Path>*   gathered_paths )
{
    assert( gathered_paths );

    if( contains( excluded_paths, in_path ) )
        return;

    if( !fs::is_directory( in_path ) )
    {
        WARNING("Path '{}' is not a directory.", in_path.string() );
        return;
    }

    for( let &entry: fs::directory_iterator( in_path ) )
    {
        if( fs::is_directory( entry ) )
            gather_files( entry.path(), extensions, excluded_paths, gathered_paths ); // Recursion, yay!

        if( fs::is_regular_file( entry )
            and contains( extensions, entry.path().extension() )
            and !contains( excluded_paths, entry.path() ) )
        {
            // LOG( "File found: {}", entry.path().string() );
            gathered_paths->push_back( entry.path() );
        }
    }
}


static constexpr fun get_compiler_name( const Compiler compiler ) -> String
{
    switch (compiler)
    {
        case Compiler::Clang: return "clang++";
        case Compiler::GCC:   WARNING( "GCC is untested at the moment." ); return "gcc";
        case Compiler::MSVC:  UNIMPLEMENTED_MSG( "No support for MSVC yet" ); return "CL";
        default: unreachable();
    }
}


static constexpr fun get_sanitizer_flags( const Target& target ) -> String
{
    var flags = String();

    if( target.sanitizers & ASan )
        flags += " -fsanitize=address";

    if( target.sanitizers & UBSan )
        flags += " -fsanitize=undefined";

    if( target.sanitizers & TSan )
        flags += " -fsanitize=thread";

    return flags;
}


static constexpr fun get_flags_from( const Target& target ) -> String
{
    var flags = String();
    for( u32 i = 0; i < target.compiler_args.size(); ++i )
    {
        flags += " -" + target.compiler_args[i];
    }
    return flags + get_sanitizer_flags( target );
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

    // TODO: Use the dependency files in `build/dep` to only compile out of date files

    for( let& source_file: cpp_paths )
    {
        assert( source_file.is_relative() );

        // FIXME: There must be a better way to do this. Perhaps with string views?
        var src_relative_cpp_path = String();
        for( let& dir: source_file )
        {
            if( dir != Path( "src" ) )
            {
                src_relative_cpp_path += "/" + dir.string();
            }
        }

        let out_obj_path = Path( obj_output_dir + src_relative_cpp_path + ".o" );
        let out_dep_path = Path( dep_output_dir + src_relative_cpp_path + ".d" );

        // Make sure we're replicating the ./src tree in the obj and deps directories
        fs::create_directories( out_obj_path.parent_path() );
        fs::create_directories( out_dep_path.parent_path() );

        var command = fmt(
            "{} -std=c++{} {} {} -O{} {} -c {} -o {} -MMD -MF {}",
            compiler_name,
            project.cpp_version,
            compiler_flags,
            defines,
            target.optimization_level,
            include_paths,
            source_file.string(),
            out_obj_path.string(),
            out_dep_path.string() );

        if( project.generate_compile_commands )
        {
            // Make sure we're replicating the ./src tree in the compile_commands.json temp directory
            let out_json_path = Path( json_temp_dir.string() + src_relative_cpp_path + ".json" );
            fs::create_directories( out_json_path.parent_path() );
            command += fmt( " -MJ {} ", out_json_path.string() );
        }

        LOG( "{}", command );

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

// FIXME: This is probably doing too many allocations by concatenating strings
static fun link( const Project& project, const Target& target ) -> ResultCode
{
    LOG("Linking...");
    var command = get_compiler_name( project.compiler );

    command += get_sanitizer_flags( target );

    // Add the compiled .o files
    {
        var obj_paths = List<Path>();
        gather_files( obj_output_dir, {".o"}, {}, &obj_paths );

        assert( !obj_paths.empty() && "No compiled binaries to link?" );

        for( let& path: obj_paths )
        {
            command += " " + path.string();
        }
    }

    for( let& dependency: project.dependencies )
    {
        if( dependency.location.type == Dependency::Location::Remote )
        {
            UNIMPLEMENTED_MSG( "No support for remote dependencies yet. Skipping {}.", dependency.name );
            continue;
        }

        // FIXME: Improve this lookup
        if( !contains(dependency.targets, "All" ) and !contains( dependency.targets, target.name ) )
            continue;

        // TODO: Include the version somehow
        if( dependency.location.path.empty() )
        {
            // system library
            command += fmt( " -l{}", dependency.name );
        }
        else
        {
            let& path = dependency.location.path; // TODO: Verify the path is in the correct subdir
            if( path.has_extension() )
            {
                if( path.extension() == ".dylib" or path.extension() == ".so" )
                {
                    // FIXME: This doesn't feel right
                    // TODO: verify the correct name with `otool`
                    fs::copy_file( path, bin_output_dir + "/" + path.filename().string() ); // So it can be loaded correctly at runtime
                } 
                command += fmt( " {}", path.string() );
            }
            else if( fs::is_directory( path ) )
            {
                command += fmt( " -L{} -l{}", path.string(), dependency.name );
            }
            else
            {
                ERROR(
                    "Dependency `{}` is not a system library or doesn't have an entry in the libraries directory.\n"
                    "Linking and/or compilation might fail.\n"
                    "Either install it, or manually create the directory `lib/{}` and place there"
                    "the binary and/or header files.",
                    dependency.name,
                    dependency.name );
                continue;
            }
        }
        command += " " + dependency.linker_flags;
    }

    command += fmt( " -o {}/{}", bin_output_dir, project.name );

    LOG( "{}", command );

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


static fun build_compile_commands_json()
{
    if( !fs::is_directory( json_temp_dir ) )
    {
        ERROR( "`build/json` directory doesn't exist." );
        return;
    }

    var json_paths = List<Path>{};
    gather_files( json_temp_dir, {".json"}, {}, &json_paths );

    var* cmp_cmd_json = fopen( "compile_commands.json", "w" );
    assert( cmp_cmd_json );
    defer( fclose( cmp_cmd_json ) );

    fputs( "[\n", cmp_cmd_json );
    defer( fputs( "]", cmp_cmd_json ) );

    for( let& entry: json_paths )
    {
        var* part = fopen( entry.c_str(), "r" );
        defer( fclose( part ) );

        // The output of -MJ for a single source file should be a single line json file
        var line_len = usize( 0 );
        char* line = nullptr;
        defer( free( line ) );

        getline( &line, &line_len, part );
        assert( line );
        assert( line_len > 0 );

        fputs( line, cmp_cmd_json );
    }
}


static fun build( const bool run_after_build ) -> ResultCode
{
    // Make sure the build directories exist
    fs::create_directories( obj_output_dir );
    fs::create_directories( dep_output_dir );
    fs::create_directories( json_temp_dir );

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
    assert( project.name != "UNNAMED" );

    if( let* target = project.find_target( project.default_target ) )
    {
        LOG( "Starting build of project \"{}\" for target \"{}\"...", project.name, target->name );

        // Find all the source files
        var cpp_paths = List<Path>{};
        gather_files( "src", {".cpp", ".cc", ".cxx"}, target->ignored_paths, &cpp_paths );

        // TODO: Fetch any remote dependencies and place them in lib/
        // TODO: Link any dynamic libraries into their corresponding .so in build/bin/

        if( let error = compile( project, *target, cpp_paths ) )
        {
            return error;
        }
        if( project.generate_compile_commands )
        {
            build_compile_commands_json();
        }
        if( let error = link( project, *target ) )
        {
            return error;
        }
        if( run_after_build )
        {
            let command = "./" + bin_output_dir + "/" + project.name;
            if( system( command.c_str() ) != OK )
            {
                return RUN_COMMAND_FAILED;
            }
        }
        return OK;
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
    fs::remove_all( "./build" );
    fs::create_directories( obj_output_dir );
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
        return build( /*run_after_build*/ false );
    }
    else if( cmd == "run" )
    {
        return build( /*run_after_build*/ true );
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


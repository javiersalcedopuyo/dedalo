#include <cstdint>
#include <cassert>

// STL bloat
#include <cstring>
#include <string>
#include <format>
#include <iostream>
#include <vector>
#include <filesystem>
#include <optional>


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

#define as static_cast


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
#if defined( ENABLE_LOGS )
    #if defined( NDEBUG )
        #define INFO(...)                println( "[DEDALO] {}",    fmt(__VA_ARGS__) )
        #define WARNING(...)
        #define ERROR(...)               println( "⛔️ [DEDALO] ERROR: {}",   fmt(__VA_ARGS__) )
        #define UNIMPLEMENTED_MSG( ... )
    #else // NDEBUG
        #define INFO(...)                println( "💬 INFO [{} @ {} ln{}]: {}",    __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
        #define WARNING(...)             println( "⚠️ WARNING [{} @ {} ln{}]: {}", __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
        #define ERROR(...)               println( "⛔️ ERROR [{} @ {} ln{}]: {}",   __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
        #define UNIMPLEMENTED_MSG( ... ) println( "🚧 UNIMPLEMENTED [{} @ {} ln{}]: {}", __func__, __FILE__, __LINE__, fmt(__VA_ARGS__) )
    #endif // NDEBUG
    #define UNIMPLEMENTED()          UNIMPLEMENTED_MSG( "" )
#else // ENABLE_LOGS
    #define INFO( ... )
    #define WARNING( ... )
    #define ERROR( ... )
    #define UNIMPLEMENTED_MSG( ... )
    #define UNIMPLEMENTED()
#endif // ENABLE_LOGS


#define unreachable() { ERROR("Unreachable point reached!"); abort(); }

static fun trim( String* input )
{
    assert( input );
    // Left trim
    {
        var iter = std::find_if_not( input->begin(), input->end(), [](char c){ return isspace(c); } );
        input->erase( input->begin(), iter );
    }
    // right trim
    {
        var iter = std::find_if_not( input->rbegin(), input->rend(), [](char c){ return isspace(c); } );
        input->erase( iter.base(), input->end() );
    }
}


static fun split( const String& input, const char delimiter ) -> List<std::string_view>
{
    var slices = List<std::string_view>{};

    var start = 0u;
    while( start < input.size() )
    {
        var pos = input.find( delimiter, start );
        if( pos == std::string_view::npos )
            pos = input.size();

        slices.emplace_back( std::string_view( input ).substr( start, pos - start ) );
        start = pos + 1;
    }
    return slices;
}


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
    struct ScriptPtr
    {
        bool (*func)() = nullptr;
        String name = "";
    };

    String       name               = "UNNAMED"; // Redundant but convenient
    u8           optimization_level = 0;
    u8           sanitizers         = No_Sanitizers;
    List<String> defines            = {};
    List<String> compiler_args      = {};
    List<Path>   ignored_paths      = {};

    List<ScriptPtr> pre_build_scripts  = {};
    List<ScriptPtr> post_build_scripts = {};
};

enum struct Location: u8
{
    System,
    Local,
    Remote,
};

enum struct Linking: u8
{
    Static,
    Dynamic,
    SingleHeader
};

struct Dependency
{
    String       name         = "UNNAMED";
    Linking      linking      = Linking::Dynamic;
    Location     location     = Location::System;
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

    // Adds a define common to all targets
    constexpr fun add_define( const String& def )
    {
        for( var& target: targets )
            target.defines.push_back( def );
    }

    // Adds a compiler argument common to all targets
    constexpr fun add_compiler_arg( const String& arg )
    {
        for( var& target: targets )
            target.compiler_args.push_back( arg );
    }

    // Adds a pre-build script common to all targets
    constexpr fun add_pre_build_script( Target::ScriptPtr script )
    {
        for( var& target: targets )
            target.pre_build_scripts.push_back( script );
    }

    // Adds a post-build script common to all targets
    constexpr fun add_post_build_script( Target::ScriptPtr script )
    {
        for( var& target: targets )
            target.post_build_scripts.push_back( script );
    }


    constexpr fun add_ignored_path( const Path& path )
    {
        for( var& target: targets )
            target.ignored_paths.push_back( path );
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
            .sanitizers         = No_Sanitizers,
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
#include <dedalo.cpp>
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


static let include_paths  = String( " -Isrc -Ilib"    ); // TODO: Make this an array of paths?
static let dep_output_dir = String( "./build/dep"     );
static let bin_output_dir = String( "./build/bin"     );
static let libraries_dir  = String( "./lib"           );
static let json_temp_dir  = Path  ( "./build/json"    );
static let obj_output_dir = String( "./build/obj" );


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
            // INFO( "File found: {}", entry.path().string() );
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
    for( let& def: target.defines )
    {
        result += " -D" + def;
    }
    return result;
}


static fun needs_recompiling(
    const Path& obj_path,
    const Path& dep_path )
-> bool
{
    if( !fs::exists( obj_path ) )
    {
        return true;
    }

    if( var* obj_dep_file = fopen( dep_path.c_str(), "r" ) )
    {
        defer( fclose( obj_dep_file ) );

        let obj_timestamp = fs::last_write_time( obj_path );

        var  dummy         = usize( 0 );
        var  is_first_line = true;
        var* line          = as< char* >( malloc( 128 * sizeof( char ) ) );
        defer( free( line ) );

        while( ( getline( &line, &dummy, obj_dep_file ) ) > 0 )
        {
            String line_str;
            line_str.assign( line );

            trim( &line_str );
            let file_paths = split( line_str, ' ' );

            for( var i = 0; i < file_paths.size(); ++i )
            {
                if( is_first_line and i == 0 )
                {
                    assert( file_paths.size() >= 2 );
                    assert( file_paths[0] == obj_path.string() + ":" );
                    is_first_line = false;
                    continue;
                }

                if( file_paths[i] == "\\" )
                    break; // EOL

                if( fs::last_write_time( file_paths[i] ) > obj_timestamp )
                    return true;
            }
        }
        return false;
    }
    return true;
}


static fun compile(
    const Project&    project,
    const Target&     target,
    const List<Path>& cpp_paths,
    const bool        force_compilation )
-> ResultCode
{
    INFO( "COMPILING..." );
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

        if( !force_compilation and !needs_recompiling( out_obj_path, out_dep_path ) )
        {
            continue;
        }

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

            // TODO: Is this necessary on Windows too?
            #if !defined( _WIN32 )
            {
                // This is unnecessary for compilation, but clangd (the LSP not the compiler)
                // doesn't seem to find system headers without it.
                command += " -I/usr/local/include";
            }
            #endif
        }

        INFO( "{}", command );

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
    INFO( "LINKING..." );
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
        if( dependency.location == Location::Remote )
        {
            UNIMPLEMENTED_MSG( "No support for remote dependencies yet. Skipping {}.", dependency.name );
            continue;
        }

        // FIXME: Improve this lookup
        if( !contains(dependency.targets, "All" ) and !contains( dependency.targets, target.name ) )
            continue;

        let lib_dir = libraries_dir + "/" + dependency.name;
        var lib_path = lib_dir + "/lib" + dependency.name;

        // TODO: Include the version somehow
        switch( dependency.linking )
        {
            case Linking::Static:
            {
                assert( fs::is_directory( lib_dir ) );

                lib_path += ".a";
                assert( fs::is_regular_file( lib_path ) );

                command += fmt( " {}", lib_path );
                break;
            }
            case Linking::Dynamic:
            {
                if( dependency.location == Location::System )
                {
                    command += fmt( " -l{}", dependency.name ); // system library
                }
                else if( dependency.location == Location::Local )
                {
                    if( fs::is_regular_file( lib_path + ".so" ) )
                    {
                        lib_path += ".so";
                    }
                    else if( fs::is_regular_file( lib_path + ".dylib" ) )
                    {
                        lib_path += ".dylib";
                    }
                    else
                    {
                        ERROR( "Dependency {} has the wrong type for a dynamic library.", dependency.name );
                        continue;
                    }

                    // So it can be loaded correctly at runtime
                    // FIXME: This relies on the install name matching the file name.
                    fs::copy_file(
                        lib_path,
                        bin_output_dir + "/" + Path( lib_path ).filename().string(),
                        fs::copy_options::overwrite_existing );

                    command += fmt( " {}", lib_path );
                }
                else
                {
                    UNIMPLEMENTED_MSG( "Remote dynamic libraries are not supported." );
                    continue;
                }
                break;
            }
            case Linking::SingleHeader:
            {
                // Nothing to link
                // TODO: If a path is provided, copy it into lib/dep_name
                continue;
            }
        }
        command += " " + dependency.linker_flags;
    }

    #if defined( __APPLE__ )
    {
        // The runtime doesn't seem to find system libraries stored in `/usr/local/lib`
        // (despite them linking fine) because that path is not in the @rpath.
        command += " -Wl,-rpath,/usr/local/lib";
    }
    #endif

    command += fmt( " -o {}/{}", bin_output_dir, project.name );

    INFO( "{}", command );

    fs::create_directories( bin_output_dir );

    if( system( command.c_str() ) != OK )
    {
        ERROR( "Failed linking.\nCommand:\n\t{}", command );
        return LINKING_FAILED;
    }
    return OK;
}


static fun compile_config( bool* has_changed ) -> ResultCode
{
    assert( has_changed );

    constant so_path  = "./build/build_script.so";
    constant cpp_path = "./build.cpp";

    if( fs::exists( so_path ) and fs::last_write_time( so_path ) >= fs::last_write_time( cpp_path ) )
    {
        *has_changed = false;
        return OK; // The binary is up to date, no need to recompile
    }
    *has_changed = true;

    INFO( "Compiling build config..." );
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


static fun build( String target_name, const bool run_after_build ) -> ResultCode
{
    // Make sure the build directories exist
    fs::create_directories( obj_output_dir );
    fs::create_directories( dep_output_dir );
    fs::create_directories( json_temp_dir );

    bool config_recompiled = false;
    if( let error = compile_config( &config_recompiled ) )
    {
        return error;
    }

    INFO( "Loading build config..." );
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

    if( target_name.empty() )
        target_name = project.default_target;

    if( let* target = project.find_target( target_name ) )
    {
        INFO( "Starting build of project \"{}\" for target \"{}\"...", project.name, target->name );

        for( u8 i = 0; i < target->pre_build_scripts.size(); ++i )
        {
            let script = target->pre_build_scripts[i];
            assert( script.func != nullptr );
            INFO( "Running pre-build script #{}: '{}' ...", i+1, script.name );
            if( script.func() == false )
            {
                ERROR( "Pre-build script #{} failed!", i );
            }
        }

        // Find all the source files
        var cpp_paths = List<Path>{};
        gather_files( "src", {".cpp", ".cc", ".cxx"}, target->ignored_paths, &cpp_paths );

        // TODO: Fetch any remote dependencies and place them in lib/
        // TODO: Link any dynamic libraries into their corresponding .so in build/bin/

        if( let error = compile( project, *target, cpp_paths, config_recompiled ) )
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

        for( u8 i = 0; i < target->post_build_scripts.size(); ++i )
        {
            let script = target->post_build_scripts[i];
            assert( script.func != nullptr );
            INFO( "Running post-build script #{}: '{}'...", i+1, script.name );
            if( script.func() == false )
            {
                ERROR( "Post-build script #{} failed!", i );
            }
        }

        if( run_after_build )
        {
            INFO( "RUNNING...\n" );
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
        let target = argc > 2 ? argv[2] : "";
        return build( target, /*run_after_build*/ false );
    }
    else if( cmd == "run" )
    {
        let target = argc > 2 ? argv[2] : "";
        return build( target, /*run_after_build*/ true );
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


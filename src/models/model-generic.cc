/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <getopt.h>

#include "thunk.hh"
#include "ggpaths.hh"
#include "path.hh"
#include "cli_description.hh"

#include "toolchain.hh"

using namespace std;
using namespace gg::thunk;

void usage( const char * argv0  )
{
  cerr << argv0 << " <cli-description> [program options...]" << endl;
}

Thunk generate_thunk( const CLIDescription & cli_description,
                      const int argc, char * argv[] )
{
  vector<struct option> long_options;
  string optstring;

  for ( const CLIOption & cli_option : cli_description.options() ) {
    if ( cli_option.long_opt.initialized() ) {
      long_options.push_back( { cli_option.long_opt->c_str(), required_argument, nullptr, cli_option.value } );
    }

    if ( cli_option.short_opt.initialized() ) {
      optstring += *cli_option.short_opt;
      optstring += ':';
    }
  }

  long_options.push_back( { nullptr, 0, nullptr, 0 } );

  optind = 1; /* reset getopt */
  opterr = 0; /* turn off error messages */

  vector<string> args = gg::models::args_to_vector( argc, argv );
  vector<InFile> infiles;
  string outfile;

  int opt;
  while ( ( opt = getopt_long( argc, argv, optstring.c_str(), long_options.data(), NULL ) ) != -1 ) {
    for ( const CLIOption & option : cli_description.options() ) {
      if ( option.value == opt ) {
        if ( option.outfile ) {
          outfile = optarg;
        }
        else {
          infiles.emplace_back( "", optarg );
        }
      }
    }
  }

  for ( const size_t idx : cli_description.infile_args() ) {
    infiles.emplace_back( "", argv[ optind + idx ] );
  }

  if ( cli_description.outfile_arg().initialized() ) {
    outfile = argv[ optind + *cli_description.outfile_arg() ];
  }

  return {
    outfile,
    { roost::rbasename( cli_description.target_bin() ).string(), args, {} },
    infiles
  };
}

int main( int argc, char * argv[] )
{
  if ( argc <= 0 ) {
    abort();
  }

  if ( argc < 2 ) {
    usage( argv[ 0 ] );
    return EXIT_FAILURE;
  }

  gg::models::init();

  CLIDescription cli_description { argv[ 1 ] };

  argv++;
  argc--;

  Thunk thunk = generate_thunk( cli_description, argc, argv );
  thunk.store();

  return 0;
}
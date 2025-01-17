/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "backend.hh"

#include <iostream>
#include <regex>
#include <stdexcept>

#include "storage/backend_local.hh"
#include "storage/backend_s3.hh"
#include "storage/backend_gs.hh"
#include "storage/backend_redis.hh"
#include "storage/backend_memcached.hh"
#include "storage/backend_crail.hh"
#include "thunk/ggutils.hh"
#include "util/digest.hh"
#include "util/optional.hh"
#include "util/uri.hh"

using namespace std;

// init static variables
string StorageBackend::backend_type_ = "";
string StorageBackend::backend_ip_ = "";
string StorageBackend::backend_port_ = "";

bool StorageBackend::is_available( const std::string & hash )
{
  return roost::exists( remote_index_path_ / hash );
}

void StorageBackend::set_available( const std::string & hash )
{
  roost::atomic_create( "", remote_index_path_ / hash );
}


unique_ptr<StorageBackend> StorageBackend::create_backend( const string & uri )
{
  ParsedURI endpoint { uri };

  unique_ptr<StorageBackend> backend;

  if ( endpoint.protocol == "s3" ) {
    backend = make_unique<S3StorageBackend>(
      ( endpoint.username.length() or endpoint.password.length() )
        ? AWSCredentials { endpoint.username, endpoint.password }
        : AWSCredentials {},
      endpoint.host,
      endpoint.options.count( "region" )
        ? endpoint.options[ "region" ]
        : "us-east-1" );
  }
  else if ( endpoint.protocol == "gs" ) {
    backend = make_unique<GoogleStorageBackend>(
      ( endpoint.username.length() or endpoint.password.length() )
        ? GoogleStorageCredentials { endpoint.username, endpoint.password }
        : GoogleStorageCredentials {},
      endpoint.host );
  }
  else if ( endpoint.protocol == "redis" ) {
    RedisClientConfig config;
    config.ip = endpoint.host;
    config.port = endpoint.port.get_or( config.port );
    config.username = endpoint.username;
    config.password = endpoint.password;

    backend = make_unique<RedisStorageBackend>( config );
  }
  else if ( endpoint.protocol == "memcached" ) {
    MemcachedClientConfig config;
    config.ip = endpoint.host;
    config.port = endpoint.port.get_or( config.port );
    config.username = endpoint.username;
    config.password = endpoint.password;

    backend = make_unique<MemcachedStorageBackend>( config );
  }
  else if ( endpoint.protocol == "crail" ) {
    CrailClientConfig config;
    config.ip = endpoint.host;
    config.port = endpoint.port.get_or( config.port );
    config.username = endpoint.username;
    config.password = endpoint.password;

    StorageBackend::backend_type_ = "crail";
    StorageBackend::backend_ip_ = endpoint.host;
    StorageBackend::backend_port_ = std::to_string(config.port);

    backend = make_unique<CrailStorageBackend>( config );
  }
  else {
    throw runtime_error( "unknown storage backend" );
  }

  if ( backend != nullptr ) {
    backend->remote_index_path_ = gg::paths::remote( digest::sha256( uri ) );
  }

  return backend;
}

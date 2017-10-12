################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################

require 'ffi'
require_relative 'ffi/version'

module Zyre
  module FFI
    module LibC
      extend ::FFI::Library
      ffi_lib ::FFI::Platform::LIBC
      attach_function :free, [ :pointer ], :void, blocking: true
    end

    extend ::FFI::Library

    def self.available?
      @available
    end

    begin
      lib_name = 'libzyre'
      lib_dirs = ['/usr/local/lib', '/opt/local/lib', '/usr/lib64']
      env_name = "#{lib_name.upcase}_PATH"
      lib_dirs = [*ENV[env_name].split(':'), *lib_dirs] if ENV[env_name]
      lib_paths = lib_dirs.map { |path| "#{path}/#{lib_name}.#{::FFI::Platform::LIBSUFFIX}" }
      ffi_lib lib_paths + [lib_name]
      @available = true
    rescue LoadError
      warn ""
      warn "WARNING: ::Zyre::FFI is not available without libzyre."
      warn ""
      @available = false
    end

    if available?
      opts = {
        blocking: true  # only necessary on MRI to deal with the GIL.
      }

      attach_function :zyre_new, [:string], :pointer, **opts
      attach_function :zyre_destroy, [:pointer], :void, **opts
      attach_function :zyre_uuid, [:pointer], :string, **opts
      attach_function :zyre_name, [:pointer], :string, **opts
      attach_function :zyre_set_name, [:pointer, :string], :void, **opts
      attach_function :zyre_set_header, [:pointer, :string, :string, :varargs], :void, **opts
      attach_function :zyre_set_verbose, [:pointer], :void, **opts
      attach_function :zyre_set_port, [:pointer, :int], :void, **opts
      attach_function :zyre_set_evasive_timeout, [:pointer, :int], :void, **opts
      attach_function :zyre_set_expired_timeout, [:pointer, :int], :void, **opts
      attach_function :zyre_set_interval, [:pointer, :size_t], :void, **opts
      attach_function :zyre_set_interface, [:pointer, :string], :void, **opts
      attach_function :zyre_set_endpoint, [:pointer, :string, :varargs], :int, **opts
      begin # DRAFT method
        attach_function :zyre_set_zcert, [:pointer, :pointer], :void, **opts
      rescue ::FFI::NotFoundError
        if $VERBOSE || $DEBUG
          warn "The DRAFT function zyre_set_zcert()" +
            " is not provided by the installed zyre library."
        end
        def self.zyre_set_zcert(*)
          raise NotImplementedError, "compile zyre with --enable-drafts"
        end
      end
      begin # DRAFT method
        attach_function :zyre_set_zap_domain, [:pointer, :string], :void, **opts
      rescue ::FFI::NotFoundError
        if $VERBOSE || $DEBUG
          warn "The DRAFT function zyre_set_zap_domain()" +
            " is not provided by the installed zyre library."
        end
        def self.zyre_set_zap_domain(*)
          raise NotImplementedError, "compile zyre with --enable-drafts"
        end
      end
      attach_function :zyre_gossip_bind, [:pointer, :string, :varargs], :void, **opts
      attach_function :zyre_gossip_connect, [:pointer, :string, :varargs], :void, **opts
      begin # DRAFT method
        attach_function :zyre_gossip_connect_curve, [:pointer, :string, :string, :varargs], :void, **opts
      rescue ::FFI::NotFoundError
        if $VERBOSE || $DEBUG
          warn "The DRAFT function zyre_gossip_connect_curve()" +
            " is not provided by the installed zyre library."
        end
        def self.zyre_gossip_connect_curve(*)
          raise NotImplementedError, "compile zyre with --enable-drafts"
        end
      end
      attach_function :zyre_start, [:pointer], :int, **opts
      attach_function :zyre_stop, [:pointer], :void, **opts
      attach_function :zyre_join, [:pointer, :string], :int, **opts
      attach_function :zyre_leave, [:pointer, :string], :int, **opts
      attach_function :zyre_recv, [:pointer], :pointer, **opts
      attach_function :zyre_whisper, [:pointer, :string, :pointer], :int, **opts
      attach_function :zyre_shout, [:pointer, :string, :pointer], :int, **opts
      attach_function :zyre_whispers, [:pointer, :string, :string, :varargs], :int, **opts
      attach_function :zyre_shouts, [:pointer, :string, :string, :varargs], :int, **opts
      attach_function :zyre_peers, [:pointer], :pointer, **opts
      attach_function :zyre_peers_by_group, [:pointer, :string], :pointer, **opts
      attach_function :zyre_own_groups, [:pointer], :pointer, **opts
      attach_function :zyre_peer_groups, [:pointer], :pointer, **opts
      attach_function :zyre_peer_address, [:pointer, :string], :pointer, **opts
      attach_function :zyre_peer_header_value, [:pointer, :string, :string], :pointer, **opts
      begin # DRAFT method
        attach_function :zyre_require_peer, [:pointer, :string, :string, :string], :int, **opts
      rescue ::FFI::NotFoundError
        if $VERBOSE || $DEBUG
          warn "The DRAFT function zyre_require_peer()" +
            " is not provided by the installed zyre library."
        end
        def self.zyre_require_peer(*)
          raise NotImplementedError, "compile zyre with --enable-drafts"
        end
      end
      attach_function :zyre_socket, [:pointer], :pointer, **opts
      attach_function :zyre_print, [:pointer], :void, **opts
      attach_function :zyre_version, [], :uint64, **opts
      attach_function :zyre_test, [:bool], :void, **opts

      require_relative 'ffi/zyre'

      attach_function :zyre_event_new, [:pointer], :pointer, **opts
      attach_function :zyre_event_destroy, [:pointer], :void, **opts
      attach_function :zyre_event_type, [:pointer], :string, **opts
      attach_function :zyre_event_peer_uuid, [:pointer], :string, **opts
      attach_function :zyre_event_peer_name, [:pointer], :string, **opts
      attach_function :zyre_event_peer_addr, [:pointer], :string, **opts
      attach_function :zyre_event_headers, [:pointer], :pointer, **opts
      attach_function :zyre_event_header, [:pointer, :string], :string, **opts
      attach_function :zyre_event_group, [:pointer], :string, **opts
      attach_function :zyre_event_msg, [:pointer], :pointer, **opts
      attach_function :zyre_event_get_msg, [:pointer], :pointer, **opts
      attach_function :zyre_event_print, [:pointer], :void, **opts
      attach_function :zyre_event_test, [:bool], :void, **opts

      require_relative 'ffi/event'
    end
  end
end

################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################

Pod::Spec.new do |s|
  s.name                 = "libbson"
  s.version              = "2.0.0-dev"
  s.summary              = "A BSON utility library."
  s.description          = <<-DESC
                           A library providing useful routines related to building,
                           parsing, and iterating BSON documents. It is a useful base
                           for those wanting to write high-performance C extensions to
                           higher level languages such as python, ruby, or perl.
                           DESC
  s.homepage             = "https://github.com/mongodb/libbson"
  s.license              = { :type => "Apache License, Version 2.0", :file => "COPYING" }
  s.author               = "10gen"
  s.social_media_url     = "http://twitter.com/mongodb"
  s.source               = { :git => "https://github.com/mongodb/libbson.git", :tag => "#{s.version}" }
  s.prepare_command      = './autogen.sh && ./configure'
  s.source_files         = "src/bson/*.{c,h}", "src/yajl/*.{c,h}"
  s.header_mappings_dir  = "src"
  s.private_header_files = "src/bson/*-private.h"
  s.compiler_flags       = "-DBSON_COMPILATION"
  s.requires_arc         = false
end

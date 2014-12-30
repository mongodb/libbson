Pod::Spec.new do |s|
  s.name                 = "libbson"
  s.version              = "1.0.2"
  s.summary              = "A BSON utility library."
  s.description          = "https://github.com/mongodb/libbson#libbson"
  s.homepage             = "https://github.com/mongodb/libbson"
  s.license              = { :type => "Apache License, Version 2.0", :file => "COPYING" }
  s.author               = "MongoDB"
  s.social_media_url     = "http://twitter.com/mongodb"
  s.source               = { :git => "https://github.com/mongodb/libbson.git", :tag => "#{s.version}" }
  s.prepare_command      = './autogen.sh && ./configure'
  s.source_files         = "src/bson/*.{c,h}", "src/yajl/*.{c,h}"
  s.header_mappings_dir  = "src"
  s.private_header_files = "src/bson/*-private.h"
  s.compiler_flags       = "-DBSON_COMPILATION"
  s.requires_arc         = false
end

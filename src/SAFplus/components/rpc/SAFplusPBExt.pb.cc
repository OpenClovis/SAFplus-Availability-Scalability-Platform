// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: SAFplusPBExt.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "SAFplusPBExt.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace SAFplus {
namespace Rpc {

namespace {


}  // namespace


void protobuf_AssignDesc_SAFplusPBExt_2eproto() {
  protobuf_AddDesc_SAFplusPBExt_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "SAFplusPBExt.proto");
  GOOGLE_CHECK(file != NULL);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_SAFplusPBExt_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
}

}  // namespace

void protobuf_ShutdownFile_SAFplusPBExt_2eproto() {
}

void protobuf_AddDesc_SAFplusPBExt_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::protobuf_AddDesc_google_2fprotobuf_2fdescriptor_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\022SAFplusPBExt.proto\022\013SAFplus.Rpc\032 googl"
    "e/protobuf/descriptor.proto:.\n\006c_type\022\035."
    "google.protobuf.FieldOptions\030\352\007 \001(\t:2\n\nc"
    "_existing\022\035.google.protobuf.FieldOptions"
    "\030\353\007 \001(\tB\t\200\001\001\210\001\000\220\001\001", 178);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "SAFplusPBExt.proto", &protobuf_RegisterTypes);
  ::google::protobuf::internal::ExtensionSet::RegisterExtension(
    &::google::protobuf::FieldOptions::default_instance(),
    1002, 9, false, false);
  ::google::protobuf::internal::ExtensionSet::RegisterExtension(
    &::google::protobuf::FieldOptions::default_instance(),
    1003, 9, false, false);
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_SAFplusPBExt_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_SAFplusPBExt_2eproto {
  StaticDescriptorInitializer_SAFplusPBExt_2eproto() {
    protobuf_AddDesc_SAFplusPBExt_2eproto();
  }
} static_descriptor_initializer_SAFplusPBExt_2eproto_;
const ::std::string c_type_default("");
::google::protobuf::internal::ExtensionIdentifier< ::google::protobuf::FieldOptions,
    ::google::protobuf::internal::StringTypeTraits, 9, false >
  c_type(kCTypeFieldNumber, c_type_default);
const ::std::string c_existing_default("");
::google::protobuf::internal::ExtensionIdentifier< ::google::protobuf::FieldOptions,
    ::google::protobuf::internal::StringTypeTraits, 9, false >
  c_existing(kCExistingFieldNumber, c_existing_default);

// @@protoc_insertion_point(namespace_scope)

}  // namespace Rpc
}  // namespace SAFplus

// @@protoc_insertion_point(global_scope)
#include <clCommon.hxx>
#include <clNameApi.hxx>

using namespace SAFplus;

Checkpoint NameRegistrar::m_checkpoint;
//TODO: initialize m_checkpoint here
//NameRegistrar::m_checkpoint.init();
SAFplus::NameRegistrar::NameRegistrar()
{
}
SAFplus::NameRegistrar::NameRegistrar(const char* name, SAFplus::Handle handle, void* object/*=NULL*/)
{
}
SAFplus::NameRegistrar::NameRegistrar(const std::string& name, SAFplus::Handle handle, void* object/*=NULL*/)
{
}
SAFplus::NameRegistrar::NameRegistrar(const char* name, SAFplus::Buffer*)
{
}
SAFplus::NameRegistrar::NameRegistrar(const std::string& name, SAFplus::Buffer*)
{
}
void SAFplus::NameRegistrar::set(const char* name, SAFplus::Handle handle, void* object/*=NULL*/)
{
}
void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Handle handle, void* object/*=NULL*/)
{
}

void SAFplus::NameRegistrar::append(const char* name, SAFplus::Handle handle, void* object/*=NULL*/)
{
}
void SAFplus::NameRegistrar::append(const std::string& name, SAFplus::Handle handle, void* object/*=NULL*/)
{
}

void SAFplus::NameRegistrar::set(const char* name, const void* data, int length)
{
}
void SAFplus::NameRegistrar::set(const std::string& name, const void* data, int length)
{
}

void SAFplus::NameRegistrar::set(const char* name, SAFplus::Buffer*)
{
}
void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Buffer*)
{
}

std::pair<SAFplus::Handle&,void*> SAFplus::NameRegistrar::get(const char* name) throw(NameException&)
{
}
std::pair<SAFplus::Handle&,void*> SAFplus::NameRegistrar::get(const std::string& name) throw(NameException&)
{
}

SAFplus::Handle& SAFplus::NameRegistrar::getHandle(const char* name) throw(NameException&)
{
}
SAFplus::Handle& SAFplus::NameRegistrar::getHandle(const std::string& name) throw(NameException&)
{
}


SAFplus::Buffer& SAFplus::NameRegistrar::getData(const char* name) throw(NameException&)
{
}
SAFplus::Buffer& SAFplus::NameRegistrar::getData(const std::string& name) throw(NameException&)
{
}
SAFplus::NameRegistrar::~NameRegistrar()
{
}

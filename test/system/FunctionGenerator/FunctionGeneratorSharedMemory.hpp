#pragma once

#include <string>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include "ParameterSet.hpp"

namespace test::system::FunctionGenerator
{
class FunctionGeneratorSharedMemory
{
    using ShmAllocator = boost::interprocess::allocator<ParameterTuple, boost::interprocess::managed_shared_memory::segment_manager>;
    using ParameterVector = boost::interprocess::vector<ParameterTuple, ShmAllocator>;
    using KeyType = std::pair<int,int>;
    using ValueType =  ParameterVector;
    using MapEntryType = std::pair<const KeyType, ValueType>;
    using MapAllocator = boost::interprocess::allocator<MapEntryType,boost::interprocess::managed_shared_memory::segment_manager>;
    using IndexMap = boost::interprocess::map<KeyType, ValueType, std::less<KeyType>, MapAllocator>;
public:
    FunctionGeneratorSharedMemory(std::string sharedMemoryName, size_t sharedMemorySizeInBytes);
    void PrepareFgData(const std::vector<ParameterTuple>& data, int fgIndex, int beam_process);

private:
    size_t m_sharedMemorySizeInBytes;
    std::string m_sharedMemoryName;
    std::unique_ptr<boost::interprocess::managed_shared_memory> m_sharedMemory;
    boost::interprocess::interprocess_mutex* m_shmMutex;
    IndexMap* m_indexMap;
};
}
#include "FunctionGeneratorSharedMemory.hpp"

using namespace test::system::FunctionGenerator;

FunctionGeneratorSharedMemory::FunctionGeneratorSharedMemory(std::string sharedMemoryName, size_t sharedMemorySizeInBytes)
    : m_sharedMemorySizeInBytes(sharedMemorySizeInBytes), m_sharedMemoryName(std::move(sharedMemoryName))
{
    boost::interprocess::shared_memory_object::remove(m_sharedMemoryName.c_str());
    // create with new size
    m_sharedMemory.reset(new boost::interprocess::managed_shared_memory(boost::interprocess::create_only,
                                                                        m_sharedMemoryName.c_str(),
                                                                        m_sharedMemorySizeInBytes));

    m_sharedMemory->zero_free_memory();
    // an int "format-version" determines the memory layout expected
    m_sharedMemory->construct<int32_t>("format-version")(0);
    // a mutex named "mutex" is required
    m_shmMutex = m_sharedMemory->construct<boost::interprocess::interprocess_mutex>("mutex")();
    // a map named "IndexMap" is required
    m_indexMap =
        m_sharedMemory->construct<IndexMap>("IndexMap")(std::less<KeyType>(), m_sharedMemory->get_segment_manager());
}

void FunctionGeneratorSharedMemory::PrepareFgData(const std::vector<ParameterTuple>& data, int fgIndex, int beam_process)
{
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*m_shmMutex);
    KeyType key{fgIndex, beam_process};
    auto foundData = m_indexMap->find(key);
    if(foundData != m_indexMap->end())
    {
        foundData->second.assign(data.cbegin(), data.cend());
        return;
    }

    ParameterVector v(data.size(), m_sharedMemory->get_segment_manager());
    v.assign(data.cbegin(), data.cend());
    m_indexMap->insert(std::pair<const KeyType, ValueType>{key, std::move(v)});
}
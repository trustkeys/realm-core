/*************************************************************************
 *
 * Copyright 2016 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#ifndef REALM_UTIL_FILE_MAPPER_HPP
#define REALM_UTIL_FILE_MAPPER_HPP

#include <realm/util/file.hpp>
#include <realm/utilities.hpp>
#include <realm/util/thread.hpp>
#include <realm/util/encrypted_file_mapping.hpp>

namespace realm {
namespace util {

void* mmap(FileDesc fd, size_t size, File::AccessMode access, size_t offset, const char* encryption_key);
void munmap(void* addr, size_t size);
void* mremap(FileDesc fd, size_t file_offset, void* old_addr, size_t old_size, File::AccessMode a, size_t new_size,
             const char* encryption_key);
void msync(FileDesc fd, void* addr, size_t size);

// A function which may be given to encryption_read_barrier. If present, the read barrier is a
// a barrier for a full array. If absent, the read barrier is a barrier only for the address
// range give as argument. If the barrier is for a full array, it will read the array header
// and determine the address range from the header.
using HeaderToSize = size_t (*)(const char* addr);
class EncryptedFileMapping;

class PageReclaimGovernor {
public:
    // Called by the page reclaimer with the current load (in bytes) and
    // must return the target load (also in bytes). Returns no_match if no
	// target can be set
	static constexpr int64_t no_match = -1;
    virtual int64_t get_current_target(size_t current_load) = 0;
};


#if REALM_ENABLE_ENCRYPTION

// Set a page reclaim governor. The governor is an object with a method which will be called periodically
// and must return a 'target' amount of memory to hold decrypted pages. The page reclaim daemon
// will then try to release pages to meet the target. The governor is called with the current
// amount of data used, for the purpose of logging - or possibly for computing the target
//
// The governor is called approximately once per second.
//
// If no governor is installed, the page reclaim daemon will not start.
void set_page_reclaim_governor(PageReclaimGovernor* governor);

void encryption_note_reader_start(SharedFileInfo& info, void* reader_id);
void encryption_note_reader_end(SharedFileInfo& info, void* reader_id);

SharedFileInfo* get_file_info_for_file(File& file);

// This variant allows the caller to obtain direct access to the encrypted file mapping
// for optimization purposes.
void* mmap(FileDesc fd, size_t size, File::AccessMode access, size_t offset, const char* encryption_key,
           EncryptedFileMapping*& mapping);

void do_encryption_read_barrier(const void* addr, size_t size, HeaderToSize header_to_size,
                                EncryptedFileMapping* mapping);

void do_encryption_write_barrier(const void* addr, size_t size, EncryptedFileMapping* mapping);

void inline encryption_read_barrier(const void* addr, size_t size, EncryptedFileMapping* mapping,
                                    HeaderToSize header_to_size = nullptr)
{
    if (mapping)
        do_encryption_read_barrier(addr, size, header_to_size, mapping);
}

void inline encryption_write_barrier(const void* addr, size_t size, EncryptedFileMapping* mapping)
{
    if (mapping)
        do_encryption_write_barrier(addr, size, mapping);
}


extern util::Mutex mapping_mutex;

inline void do_encryption_read_barrier(const void* addr, size_t size, HeaderToSize header_to_size,
                                       EncryptedFileMapping* mapping)
{
    UniqueLock lock(mapping_mutex);
    mapping->read_barrier(addr, size, header_to_size);
}

inline void do_encryption_write_barrier(const void* addr, size_t size, EncryptedFileMapping* mapping)
{
    LockGuard lock(mapping_mutex);
    mapping->write_barrier(addr, size);
}

#else
void inline encryption_read_barrier(const void*, size_t, EncryptedFileMapping*, HeaderToSize header_to_size = nullptr)
{
    static_cast<void>(header_to_size);
}
void inline encryption_write_barrier(const void*, size_t)
{
}
void inline encryption_write_barrier(const void*, size_t, EncryptedFileMapping*)
{
}

void set_page_reclaim_governor(PageReclaimGovernor*)
{
}

void encryption_note_reader_start(SharedFileInfo&, void*)
{
}

void encryption_note_reader_end(SharedFileInfo&, void*)
{
}

#endif

// helpers for encrypted Maps
template <typename T>
void encryption_read_barrier(File::Map<T>& map, size_t index, size_t num_elements = 1)
{
    T* addr = map.get_addr();
    encryption_read_barrier(addr + index, sizeof(T) * num_elements, map.get_encrypted_mapping());
}

template <typename T>
void encryption_write_barrier(File::Map<T>& map, size_t index, size_t num_elements = 1)
{
    T* addr = map.get_addr();
    encryption_write_barrier(addr + index, sizeof(T) * num_elements, map.get_encrypted_mapping());
}

File::SizeType encrypted_size_to_data_size(File::SizeType size) noexcept;
File::SizeType data_size_to_encrypted_size(File::SizeType size) noexcept;

size_t round_up_to_page_size(size_t size) noexcept;
}
}
#endif

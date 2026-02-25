#include "shard_storage.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace plexome {

    ShardStorage::ShardStorage(const std::filesystem::path& base_path) : root_dir_(base_path) {
        if (!std::filesystem::exists(root_dir_)) {
            std::filesystem::create_directories(root_dir_);
        }
        std::cout << "[Storage] Root directory set to: " << root_dir_ << std::endl;
    }

    std::string ShardStorage::shard_to_hex(const ShardID& id) const {
        std::stringstream ss;
        for (auto byte : id) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        return ss.str();
    }

    std::filesystem::path ShardStorage::get_path(const ShardID& id) const {
        return root_dir_ / (shard_to_hex(id) + ".pxm");
    }

    bool ShardStorage::save_shard(const ShardID& id, const std::vector<uint8_t>& data) {
        auto path = get_path(id);
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) return false;

        ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
        std::cout << "[Storage] Saved shard: " << shard_to_hex(id) << " (" << data.size() << " bytes)" << std::endl;
        return true;
    }

    std::optional<std::vector<uint8_t>> ShardStorage::load_shard(const ShardID& id) {
        auto path = get_path(id);
        if (!std::filesystem::exists(path)) return std::nullopt;

        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (ifs.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return buffer;
        }
        return std::nullopt;
    }

    bool ShardStorage::exists(const ShardID& id) const {
        return std::filesystem::exists(get_path(id));
    }
}

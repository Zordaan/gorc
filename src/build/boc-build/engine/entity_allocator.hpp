#pragma once

#include "base_file_entity.hpp"
#include "log/log.hpp"
#include <memory>
#include <type_traits>
#include <vector>
#include <map>
#include <mutex>

namespace gorc {

    class entity_allocator {
    private:
        std::mutex atomic_allocate_lock;
        std::vector<std::unique_ptr<entity>> entities;
        std::map<path, entity*> file_map;

        template <typename T, typename ...ArgT>
        T* inner_emplace(ArgT &&...args)
        {
            auto ptr = std::make_unique<T>(std::forward<ArgT>(args)...);
            T *rv = ptr.get();
            entities.push_back(std::move(ptr));
            return rv;
        }

    public:

        template <typename T, typename ...ArgT>
        typename std::enable_if<std::is_base_of<base_file_entity, T>::value, T*>::type
            emplace(path const &name, ArgT &&...args)
        {
            std::lock_guard<std::mutex> lg(atomic_allocate_lock);

            // Return existing file entity
            auto it = file_map.find(name);
            if(it != file_map.end()) {
                T *val = dynamic_cast<T*>(it->second);
                if(val == nullptr) {
                    LOG_FATAL(format("entity '%s' type mismatch") % it->second->name());
                }

                return val;
            }

            T *ent = inner_emplace<T, path const &, ArgT...>(name, std::forward<ArgT>(args)...);
            file_map.emplace(name, ent);
            return ent;
        }

        template <typename T>
        typename std::enable_if<std::is_base_of<base_file_entity, T>::value, T*>::type
            emplace(entity_input_stream &is)
        {
            std::lock_guard<std::mutex> lg(atomic_allocate_lock);

            // Always assume deserialized graph is normalized
            T *ent = inner_emplace<T>(is);
            file_map.emplace(ent->file_path(), ent);
            return ent;
        }

        template <typename T, typename ...ArgT>
        typename std::enable_if<!std::is_base_of<base_file_entity, T>::value, T*>::type
            emplace(ArgT &&...args)
        {
            std::lock_guard<std::mutex> lg(atomic_allocate_lock);

            return inner_emplace<T, ArgT...>(std::forward<ArgT>(args)...);
        }

        void clear();

        template <typename T, typename ...ArgT>
        std::tuple<T*, bool> maybe_emplace_file(path const &name, ArgT &&...args)
        {
            std::lock_guard<std::mutex> lg(atomic_allocate_lock);

            auto it = file_map.find(name);
            if(it != file_map.end()) {
                T *val = dynamic_cast<T*>(it->second);
                if(val == nullptr) {
                    LOG_FATAL(format("entity '%s' type mismatch") % it->second->name());
                }

                return std::make_tuple(val, false);
            }

            T *ent = inner_emplace<T, path const &, ArgT...>(name, std::forward<ArgT>(args)...);
            file_map.emplace(name, ent);
            return std::make_tuple(ent, true);
        }
    };

}

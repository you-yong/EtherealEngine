#pragma once

#include <functional>
#include <unordered_map>

#include "core/common/nonstd/type_traits.hpp"
#include "core/common/string.h"

#include "core/subsystem/tasks.hpp"
#include "core/filesystem/filesystem.h"
#include "asset_flags.h"
#include "asset_extensions.h"

namespace runtime
{
	/// aliases
	template<typename T>
	using request_container_t = std::unordered_map<std::string, core::task_future<asset_handle<T>>>;

	struct base_storage
	{
		//-----------------------------------------------------------------------------
		//  Name : ~base_storage (virtual )
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		virtual ~base_storage() = default;

		//-----------------------------------------------------------------------------
		//  Name : clear (virtual )
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		virtual void clear() = 0;

		//-----------------------------------------------------------------------------
		//  Name : clear (virtual )
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		virtual void clear(const std::string& protocol) = 0;

	};

	template<typename T>
	struct asset_storage : public base_storage
	{
		//-----------------------------------------------------------------------------
		//  Name : ~storage ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		~asset_storage() = default;

		//-----------------------------------------------------------------------------
		//  Name : clear ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		void clear()
		{
			container.clear();
		}

		//-----------------------------------------------------------------------------
		//  Name : clear ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		void clear(const std::string& protocol)
		{
			auto container_copy = container;
			for (const auto& pair : container_copy)
			{
				const auto& id = pair.first;
				const auto& task = pair.second;

				if (string_utils::begins_with(id, protocol, true))
				{
					task.wait();
					container.erase(id);
				}
			}
		}

		/// key, data, size
		delegate<core::task_future<asset_handle<T>>(const std::string&, const std::uint8_t*, std::uint32_t)> load_from_memory;

		/// key, mode
		delegate<core::task_future<asset_handle<T>>(const std::string&, const load_mode&, asset_handle<T>)> load_from_file;

		/// key, mode
		delegate<core::task_future<asset_handle<T>>(const std::string&, std::shared_ptr<T>)> load_from_instance;

		/// key, asset
		delegate<void(const std::string&, const asset_handle<T>&)> save_to_file;

		/// key, new_key
		delegate<void(const std::string&, const std::string&)> rename_asset_file;

		/// key
		delegate<void(const std::string&)> delete_asset_file;

		/// Storage container
		request_container_t<T> container;
	};

	class asset_manager : public core::subsystem
	{
	public:
		bool initialize();

		//-----------------------------------------------------------------------------
		//  Name : clear ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		void clear()
		{
			for (auto& pair : _storages)
			{
				auto& storage = pair.second;
				storage->clear();
			}
		}

		//-----------------------------------------------------------------------------
		//  Name : clear ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		void clear(const std::string& protocol)
		{
			for (auto& pair : _storages)
			{
				auto& storage = pair.second;
				storage->clear(protocol);
			}
		}
		//-----------------------------------------------------------------------------
		//  Name : add_storage ()
		/// <summary>
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		template <typename S, typename ... Args>
		asset_storage<S>& add_storage(Args&& ... args)
		{
			auto operation = _storages.emplace
			(
				rtti::type_id<asset_storage<S>>().hash_code(),
				std::make_unique<asset_storage<S>>(std::forward<Args>(args) ...)
			);

			return static_cast<asset_storage<S>&>(*operation.first->second);
		}

		template<typename T>
		core::task_future<asset_handle<T>> load(
			const std::string& key,
			load_mode mode = load_mode::sync,
            load_flags flags = load_flags::standard)
		{
			auto& storage = get_storage<T>();
			//if embedded resource
			if (key.find("embedded") != std::string::npos)
			{
				return find_asset_impl<T>(key, storage.container);
			}
			else
			{
				return load_asset_from_file_impl<T>(key, mode, flags, storage.container, storage.load_from_file);

			}
		}

		//-----------------------------------------------------------------------------
		//  Name : create_asset_from_memory ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		template<typename T>
		core::task_future<asset_handle<T>> create_asset_from_memory(
			const std::string& key,
			const std::uint8_t* data,
			const std::uint32_t& size,
			load_mode mode = load_mode::sync,
            load_flags flags = load_flags::standard)
		{
			auto& storage = get_storage<T>();
			return create_asset_from_memory_impl<T>(
				key,
				data,
				size,
				mode,
				flags,
				storage.container,
				storage.load_from_memory);
		}

		template<typename T>
		core::task_future<asset_handle<T>> find_asset_entry(const std::string& key)
		{
			auto& storage = get_storage<T>();
			return find_asset_impl<T>(key, storage.container);
		}
		
		template<typename T>
		core::task_future<asset_handle<T>> load_asset_from_instance(const std::string& key, std::shared_ptr<T> entry)
		{
			auto& storage = get_storage<T>();
			return load_asset_from_instance_impl(key, entry, storage.container, storage.load_from_instance);
		}

		template<typename T>
		void rename_asset(const std::string& key, const std::string& new_key)
		{
			auto& storage = get_storage<T>();
			storage.rename_asset_file(key, new_key);
		
			auto it = storage.container.find(key);
			if (it != storage.container.end())
			{
				auto& future = it->second;
				auto asset = future.get();
				asset.link->id = new_key;
				storage.container[new_key] = future;
				storage.container.erase(it);
			}
		}
		
		template<typename T>
		void clear_asset(const std::string& key)
		{
			auto& storage = get_storage<T>();

			auto it = storage.container.find(key);
			if (it != storage.container.end())
			{
				auto& future = it->second;

				auto asset = future.get();
				asset.link->asset.reset();
				asset.link->id.clear();

				storage.container.erase(it);
			}
		}
		//-----------------------------------------------------------------------------
		//  Name : delete_asset ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		template<typename T>
		void delete_asset(const std::string& key)
		{
			auto& storage = get_storage<T>();
			storage.delete_asset_file(key);
		
			clear_asset<T>(key);
		}
				
		//-----------------------------------------------------------------------------
		//  Name : save ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		template<typename T>
		void save(const asset_handle<T>& asset)
		{
			auto& storage = get_storage<T>();
			storage.save_to_file(asset.id(), asset);
		}
	private:
		//-----------------------------------------------------------------------------
		//  Name : load_asset_from_file_impl ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		template<typename T, typename F>
		core::task_future<asset_handle<T>>& load_asset_from_file_impl(
			const std::string& key,
			load_mode mode,
			load_flags flags,
			request_container_t<T>& container,
			F&& load_func
		)
		{
			fs::error_code err;
			auto it = container.find(key);
			if (it != std::end(container))
			{
				auto& future = it->second;

				if (flags == load_flags::reload && future.is_ready())
				{
					asset_handle<T> original = future.get();
					future = load_func(key, mode, original);
				}

				if (mode == load_mode::sync)
				{
					future.wait();
				}

				return future;
			}
			else
			{
				auto& future = container[key];
				//Dispatch the loading
				asset_handle<T> original;
				future = load_func(key, mode, original);

				return future;
			}
		}

		//-----------------------------------------------------------------------------
		//  Name : create_asset_from_memory_impl ()
		/// <summary>
		/// 
		/// 
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		template<typename T, typename F>
		core::task_future<asset_handle<T>>& create_asset_from_memory_impl(
			const std::string& key,
			const std::uint8_t* data,
			const std::uint32_t& size,
			load_mode mode,
			load_flags flags,
			request_container_t<T>& container,
			F&& load_func
		)
		{

			auto it = container.find(key);
			if (it != std::end(container))
			{
				// If there is already a loading request.
				auto& future = it->second;
				return future;
			}
			else
			{
				auto& future = container[key];
				//Dispatch the loading
				future = load_func(key, data, size);

				return future;
			}

		}


		template<typename T, typename F>
		core::task_future<asset_handle<T>>& load_asset_from_instance_impl(
			const std::string& key,	
			std::shared_ptr<T> entry,
			request_container_t<T>& container,
			F&& load_func)
		{

			auto& future = container[key];
			//Dispatch the loading
			future = load_func(key, entry);

			return future;
		}

		template<typename T>
		core::task_future<asset_handle<T>> find_asset_impl(
			const std::string& key,
			request_container_t<T>& container
		)
		{
			auto it = container.find(key);
			if (it != container.end())
			{
				return it->second;
			}
			else
			{
				return core::task_future<asset_handle<T>>();
			}
		}

		//-----------------------------------------------------------------------------
		//  Name : get_storage ()
		/// <summary>
		/// 
		/// </summary>
		//-----------------------------------------------------------------------------
		template <typename S>
		asset_storage<S>& get_storage()
		{
			auto it = _storages.find(rtti::type_id<asset_storage<S>>().hash_code());
			assert(it != _storages.end());
			return (static_cast<asset_storage<S>&>(*it->second.get()));
		}
		/// Different storages
		std::unordered_map<std::size_t, std::unique_ptr<base_storage>> _storages;
	};
}
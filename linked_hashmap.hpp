/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
	class Key,
	class T,
	class Hash = std::hash<Key>, 
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;
 
	// Forward declarations
	struct ListNode;
	struct Node;

	// List node for insertion order - separate from hash table nodes
	struct ListNode {
		Node *hash_node;    // pointer to the actual hash table node
		ListNode *next;     // next in insertion order
		ListNode *prev;     // prev in insertion order
		
		ListNode() : hash_node(nullptr), next(nullptr), prev(nullptr) {}
	};

	// Node in the hash table
	struct Node {
		value_type *data;
		Node *next;         // next in hash bucket
		Node *prev;         // prev in hash bucket
		ListNode *list_node; // corresponding list node
		
		Node(const Key &k, const T &v, ListNode *ln) : next(nullptr), prev(nullptr), list_node(ln) {
			data = new value_type(k, v);
		}
		~Node() {
			delete data;
		}
	};

private:
	Node **buckets;       // hash table buckets
	size_t bucket_count;  // number of buckets
	size_t num_elements;  // number of elements
	ListNode *head;       // head of insertion order list (dummy)
	ListNode *tail;       // tail of insertion order list (dummy)
	
	Hash hash_func;
	Equal equal_func;

	static constexpr double MAX_LOAD_FACTOR = 0.75;
	static constexpr size_t INITIAL_BUCKET_COUNT = 16;

	void init() {
		bucket_count = INITIAL_BUCKET_COUNT;
		buckets = new Node*[bucket_count]();
		num_elements = 0;
		head = new ListNode();
		tail = new ListNode();
		head->next = tail;
		tail->prev = head;
	}

	void clear_buckets() {
		for (size_t i = 0; i < bucket_count; ++i) {
			Node *cur = buckets[i];
			while (cur != nullptr) {
				Node *next = cur->next;
				delete cur;
				cur = next;
			}
		}
		delete[] buckets;
	}

	void rehash() {
		if (num_elements < bucket_count * MAX_LOAD_FACTOR) return;
		
		size_t old_bucket_count = bucket_count;
		Node **old_buckets = buckets;
		
		bucket_count *= 2;
		buckets = new Node*[bucket_count]();
		
		for (size_t i = 0; i < old_bucket_count; ++i) {
			Node *cur = old_buckets[i];
			while (cur != nullptr) {
				Node *next = cur->next;
				size_t new_idx = hash_func(cur->data->first) % bucket_count;
				cur->next = buckets[new_idx];
				cur->prev = nullptr;
				if (buckets[new_idx] != nullptr) {
					buckets[new_idx]->prev = cur;
				}
				buckets[new_idx] = cur;
				cur = next;
			}
		}
		delete[] old_buckets;
	}

	Node* find_node(const Key &key) const {
		size_t idx = hash_func(key) % bucket_count;
		Node *cur = buckets[idx];
		while (cur != nullptr) {
			if (equal_func(cur->data->first, key)) {
				return cur;
			}
			cur = cur->next;
		}
		return nullptr;
	}

	void insert_to_list(ListNode *list_node, Node *hash_node) {
		list_node->hash_node = hash_node;
		list_node->prev = tail->prev;
		list_node->next = tail;
		tail->prev->next = list_node;
		tail->prev = list_node;
	}

	void remove_from_list(ListNode *list_node) {
		list_node->prev->next = list_node->next;
		list_node->next->prev = list_node->prev;
	}

	void copy_from(const linked_hashmap &other) {
		for (ListNode *cur = other.head->next; cur != other.tail; cur = cur->next) {
			insert(*(cur->hash_node->data));
		}
	}

public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		const linked_hashmap *map;
		ListNode *node;
	public:
		friend class const_iterator;
		friend class linked_hashmap;
		
		// The following code is written for the C++ type_traits library.
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;

		iterator(const linked_hashmap *m = nullptr, ListNode *n = nullptr) : map(m), node(n) {}
		iterator(const iterator &other) : map(other.map), node(other.node) {}
		
		iterator operator++(int) {
			iterator tmp = *this;
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->next;
			return tmp;
		}
		
		iterator & operator++() {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->next;
			return *this;
		}
		
		iterator operator--(int) {
			iterator tmp = *this;
			if (node == nullptr || node == map->head->next) {
				throw invalid_iterator();
			}
			node = node->prev;
			return tmp;
		}
		
		iterator & operator--() {
			if (node == nullptr || node == map->head->next) {
				throw invalid_iterator();
			}
			node = node->prev;
			return *this;
		}
		
		value_type & operator*() const {
			if (node == nullptr || node == map->head || node == map->tail || node->hash_node == nullptr) {
				throw invalid_iterator();
			}
			return *(node->hash_node->data);
		}
		
		bool operator==(const iterator &rhs) const {
			return node == rhs.node;
		}
		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node;
		}
		bool operator!=(const iterator &rhs) const {
			return node != rhs.node;
		}
		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.node;
		}

		value_type* operator->() const {
			if (node == nullptr || node == map->head || node == map->tail || node->hash_node == nullptr) {
				throw invalid_iterator();
			}
			return node->hash_node->data;
		}
	};
 
	class const_iterator {
	private:
		const linked_hashmap *map;
		const ListNode *node;
	public:
		friend class iterator;
		friend class linked_hashmap;
		
		using difference_type = std::ptrdiff_t;
		using value_type = const typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;

		const_iterator(const linked_hashmap *m = nullptr, const ListNode *n = nullptr) : map(m), node(n) {}
		const_iterator(const const_iterator &other) : map(other.map), node(other.node) {}
		const_iterator(const iterator &other) : map(other.map), node(other.node) {}
		
		const_iterator operator++(int) {
			const_iterator tmp = *this;
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->next;
			return tmp;
		}
		
		const_iterator & operator++() {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->next;
			return *this;
		}
		
		const_iterator operator--(int) {
			const_iterator tmp = *this;
			if (node == nullptr || node == map->head->next) {
				throw invalid_iterator();
			}
			node = node->prev;
			return tmp;
		}
		
		const_iterator & operator--() {
			if (node == nullptr || node == map->head->next) {
				throw invalid_iterator();
			}
			node = node->prev;
			return *this;
		}
		
		const value_type & operator*() const {
			if (node == nullptr || node == map->head || node == map->tail || node->hash_node == nullptr) {
				throw invalid_iterator();
			}
			return *(node->hash_node->data);
		}
		
		const value_type* operator->() const {
			if (node == nullptr || node == map->head || node == map->tail || node->hash_node == nullptr) {
				throw invalid_iterator();
			}
			return node->hash_node->data;
		}
		
		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node;
		}
		bool operator==(const iterator &rhs) const {
			return node == rhs.node;
		}
		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.node;
		}
		bool operator!=(const iterator &rhs) const {
			return node != rhs.node;
		}
	};
 
	linked_hashmap() {
		init();
	}
	
	linked_hashmap(const linked_hashmap &other) {
		init();
		copy_from(other);
	}
 
	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this == &other) return *this;
		clear();
		clear_buckets();
		delete head;
		delete tail;
		head = new ListNode();
		tail = new ListNode();
		head->next = tail;
		tail->prev = head;
		bucket_count = INITIAL_BUCKET_COUNT;
		buckets = new Node*[bucket_count]();
		num_elements = 0;
		copy_from(other);
		return *this;
	}
 
	~linked_hashmap() {
		clear();
		clear_buckets();
		delete head;
		delete tail;
	}
 
	T & at(const Key &key) {
		Node *node = find_node(key);
		if (node == nullptr) {
			throw index_out_of_bound();
		}
		return node->data->second;
	}
	
	const T & at(const Key &key) const {
		Node *node = find_node(key);
		if (node == nullptr) {
			throw index_out_of_bound();
		}
		return node->data->second;
	}
 
	T & operator[](const Key &key) {
		Node *node = find_node(key);
		if (node == nullptr) {
			// Insert new element with default value
			auto result = insert(value_type(key, T()));
			return result.first.node->hash_node->data->second;
		}
		return node->data->second;
	}
 
	const T & operator[](const Key &key) const {
		return at(key);
	}
 
	iterator begin() {
		return iterator(this, head->next);
	}
	
	const_iterator cbegin() const {
		return const_iterator(this, head->next);
	}
 
	iterator end() {
		return iterator(this, tail);
	}
	
	const_iterator cend() const {
		return const_iterator(this, tail);
	}
 
	bool empty() const {
		return num_elements == 0;
	}
 
	size_t size() const {
		return num_elements;
	}
 
	void clear() {
		// Clear all list nodes and hash nodes
		ListNode *cur = head->next;
		while (cur != tail) {
			ListNode *next = cur->next;
			if (cur->hash_node != nullptr) {
				// Find and remove from hash bucket
				Node *hn = cur->hash_node;
				size_t idx = hash_func(hn->data->first) % bucket_count;
				if (buckets[idx] == hn) {
					buckets[idx] = hn->next;
				}
				if (hn->prev != nullptr) {
					hn->prev->next = hn->next;
				}
				if (hn->next != nullptr) {
					hn->next->prev = hn->prev;
				}
				delete hn;
			}
			delete cur;
			cur = next;
		}
		// Reset buckets
		for (size_t i = 0; i < bucket_count; ++i) {
			buckets[i] = nullptr;
		}
		num_elements = 0;
		head->next = tail;
		tail->prev = head;
	}
 
	pair<iterator, bool> insert(const value_type &value) {
		Node *existing = find_node(value.first);
		if (existing != nullptr) {
			// Find corresponding list node
			return pair<iterator, bool>(iterator(this, existing->list_node), false);
		}
		
		rehash();
		
		// Create new list node first
		ListNode *ln = new ListNode();
		Node *new_node = new Node(value.first, value.second, ln);
		ln->hash_node = new_node;
		
		size_t idx = hash_func(value.first) % bucket_count;
		
		// Insert into hash bucket
		new_node->next = buckets[idx];
		new_node->prev = nullptr;
		if (buckets[idx] != nullptr) {
			buckets[idx]->prev = new_node;
		}
		buckets[idx] = new_node;
		
		// Insert into list
		insert_to_list(ln, new_node);
		
		num_elements++;
		return pair<iterator, bool>(iterator(this, ln), true);
	}
 
	void erase(iterator pos) {
		if (pos.node == nullptr || pos.node == tail || pos.node == head) {
			throw invalid_iterator();
		}
		
		ListNode *ln = pos.node;
		Node *hn = ln->hash_node;
		
		if (hn == nullptr) {
			throw invalid_iterator();
		}
		
		// Remove from hash bucket
		if (hn->prev != nullptr) {
			hn->prev->next = hn->next;
		} else {
			size_t idx = hash_func(hn->data->first) % bucket_count;
			buckets[idx] = hn->next;
		}
		if (hn->next != nullptr) {
			hn->next->prev = hn->prev;
		}
		
		// Remove from list
		remove_from_list(ln);
		
		delete hn;
		delete ln;
		num_elements--;
	}
 
	size_t count(const Key &key) const {
		return find_node(key) != nullptr ? 1 : 0;
	}
 
	iterator find(const Key &key) {
		Node *node = find_node(key);
		if (node == nullptr) {
			return end();
		}
		return iterator(this, node->list_node);
	}
	
	const_iterator find(const Key &key) const {
		Node *node = find_node(key);
		if (node == nullptr) {
			return cend();
		}
		return const_iterator(this, node->list_node);
	}
};

}

#endif

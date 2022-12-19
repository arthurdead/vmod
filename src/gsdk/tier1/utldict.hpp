#pragma once

#include "utlmemory.hpp"

namespace gsdk
{
	template <typename I>
	struct UtlRBTreeLinks_t
	{
		I m_Left;
		I m_Right;
		I m_Parent;
		I m_Tag;
	};

	template <typename T, typename I>
	struct UtlRBTreeNode_t : public UtlRBTreeLinks_t<I>
	{
		T m_Data;
	};

	template <typename T, typename I = unsigned short, typename L = bool(*)(const T &, const T &), typename M = CUtlMemory<UtlRBTreeNode_t<T, I>, I>>
	class CUtlRBTree
	{
	public:
		void erase(std::size_t i) noexcept;

		using links_t = UtlRBTreeLinks_t<I>;

		void unlink(std::size_t i) noexcept;
		void free_node(std::size_t i) noexcept;

		enum color_t : int
		{
			red,
			black
		};

		using key_type = typename T::key_type;

		static constexpr std::size_t npos{M::npos};

		std::size_t find(const key_type &key) const noexcept;

		inline void set_left_child(std::size_t i, std::size_t child) noexcept
		{ m_Elements[i].m_Left = static_cast<I>(child); }
		inline void set_right_child(std::size_t i, std::size_t child) noexcept
		{ m_Elements[i].m_Right = static_cast<I>(child); }
		inline void set_parent(std::size_t i, std::size_t parent) noexcept
		{ m_Elements[i].m_Parent = static_cast<I>(parent); }
		inline void set_color(std::size_t i, color_t clr) noexcept
		{ m_Elements[i].m_Tag = static_cast<I>(clr); }
		inline void set_root(std::size_t i) noexcept
		{ m_Root = static_cast<I>(i); }

		inline bool is_black(std::size_t i) const noexcept
		{ return m_Elements[i].m_Tag == static_cast<I>(color_t::black); }
		inline bool is_red(std::size_t i) const noexcept
		{ return m_Elements[i].m_Tag == static_cast<I>(color_t::red); }
		inline color_t color(std::size_t i) const noexcept
		{ return static_cast<color_t>(m_Elements[i].m_Tag); }

		inline std::size_t child_parent(std::size_t i) const noexcept
		{ return static_cast<std::size_t>(m_Elements[i].m_Parent); }
		inline std::size_t left_child(std::size_t i) const noexcept
		{ return static_cast<std::size_t>(m_Elements[i].m_Left); }
		inline std::size_t right_child(std::size_t i) const noexcept
		{ return static_cast<std::size_t>(m_Elements[i].m_Right); }

		inline bool is_left_child(std::size_t i) const noexcept
		{ return m_Elements[m_Elements[i].m_Parent].m_Left == static_cast<I>(i); }
		inline bool is_right_child(std::size_t i) const noexcept
		{ return m_Elements[m_Elements[i].m_Parent].m_Right == static_cast<I>(i); }
		inline bool is_root(std::size_t i) const noexcept
		{ return static_cast<I>(i) == m_Root; }

		inline const T &operator[](std::size_t i) const noexcept
		{ return m_Elements[i].m_Data; }
		inline const T &element(std::size_t i) const noexcept
		{ return m_Elements[i].m_Data; }

		void rotate_left(std::size_t i) noexcept;
		void rotate_right(std::size_t i) noexcept;
		void remove_rebalance(std::size_t i) noexcept;

		using less_func_t = L;

		less_func_t m_LessFunc;

		using node_t = UtlRBTreeNode_t<T, I>;

		M m_Elements;
		I m_Root;
		I m_NumElements;
		I m_FirstFree;
		typename M::iterator m_LastAlloc;

		node_t *m_pElements;
	};

	template <typename K, typename T, typename I = unsigned short>
	class CUtlMap
	{
	public:
		struct node_t
		{
			using key_type = K;
			using element_type = T;

			K key;
			T elem;
		};

		using tree_t = CUtlRBTree<node_t, I>;

		static constexpr std::size_t npos{tree_t::npos};

		inline const K &key(std::size_t i) const noexcept
		{ return m_Tree[i].key; }

		inline std::size_t find(const K &key) const noexcept
		{ return m_Tree.find(key); }

		inline void erase(std::size_t i)
		{ m_Tree.erase(i); }

		tree_t m_Tree;
	};

	template <typename T, typename I = int>
	class CUtlDict
	{
	public:
		using map_t = CUtlMap<const char *, T, I>;

		static constexpr std::size_t npos{map_t::npos};

		inline std::size_t find(std::string_view name) const noexcept
		{ return m_Elements.find(name.data()); }

		void erase(std::size_t i) noexcept;

		map_t m_Elements;
	};
}

#include "utldict.tpp"

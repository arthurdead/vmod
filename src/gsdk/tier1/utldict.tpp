namespace gsdk
{
	template <typename T, typename I>
	void CUtlDict<T, I>::erase(std::size_t i) noexcept
	{
		std::free(const_cast<char *>(m_Elements.key(i)));
		m_Elements.erase(i);
	}

	template <typename T, typename I, typename L, typename M>
	void CUtlRBTree<T, I, L, M>::rotate_left(std::size_t i) noexcept
	{
		std::size_t rightchild{right_child(i)};
		set_right_child(i, left_child(rightchild));

		if(left_child(rightchild) != M::npos) {
			set_parent(left_child(rightchild), i);
		}

		if(rightchild != M::npos) {
			set_parent(rightchild, child_parent(i));
		}

		if(!is_root(i)) {
			if(is_left_child(i)) {
				set_left_child(child_parent(i), rightchild);
			} else {
				set_right_child(child_parent(i), rightchild);
			}
		} else {
			set_root(rightchild);
		}

		set_left_child(rightchild, i);

		if(i != M::npos) {
			set_parent(i, rightchild);
		}
	}

	template <typename T, typename I, typename L, typename M>
	void CUtlRBTree<T, I, L, M>::rotate_right(std::size_t i) noexcept
	{
		std::size_t leftchild{left_child(i)};
		set_left_child(i, right_child(leftchild));

		if(right_child(leftchild) != M::npos) {
			set_parent(right_child(leftchild), i);
		}

		if(leftchild != M::npos) {
			set_parent(leftchild, child_parent(i));
		}

		if(!is_root(i)) {
			if(is_right_child(i)) {
				set_right_child(child_parent(i), leftchild);
			} else {
				set_left_child(child_parent(i), leftchild);
			}
		} else {
			set_root(leftchild);
		}

		set_right_child(leftchild, i);

		if(i != M::npos) {
			set_parent(i, leftchild);
		}
	}

	template <typename T, typename I, typename L, typename M>
	void CUtlRBTree<T, I, L, M>::remove_rebalance(std::size_t i) noexcept
	{
		while(i != m_Root && is_black(i)) {
			std::size_t parent{child_parent(i)};

			if(i == left_child(parent)) {
				std::size_t sibling{right_child(parent)};
				if(is_red(sibling)) {
					set_color(sibling, color_t::black);
					set_color(parent, color_t::red);
					rotate_left(parent);

					parent = child_parent(i);
					sibling = right_child(parent);
				}

				if((is_black(left_child(sibling))) && (is_black(right_child(sibling)))) {
					if(sibling != M::npos) {
						set_color(sibling, color_t::red);
					}

					i = parent;
				} else {
					if(is_black(right_child(sibling))) {
						set_color(left_child(sibling), color_t::black);
						set_color(sibling, color_t::red);
						rotate_right(sibling);

						parent = child_parent(i);
						sibling = right_child(parent);
					}

					set_color(sibling, color(parent));
					set_color(parent, color_t::black);
					set_color(right_child(sibling), color_t::black);
					rotate_left(parent);
					i = m_Root;
				}
			} else {
				std::size_t sibling = left_child(parent);
				if(is_red(sibling)) {
					set_color(sibling, color_t::black);
					set_color(parent, color_t::red);
					rotate_right(parent);

					parent = child_parent(i);
					sibling = left_child(parent);
				}

				if((is_black(right_child(sibling))) && (is_black(left_child(sibling)))) {
					if(sibling != M::npos) {
						set_color(sibling, color_t::red);
					}

					i = parent;
				} else {
					if(is_black(left_child(sibling))) {
						set_color(right_child(sibling), color_t::black);
						set_color(sibling, color_t::red);
						rotate_left(sibling);

						parent = child_parent(i);
						sibling = left_child(parent);
					}

					set_color(sibling, color(parent));
					set_color(parent, color_t::black);
					set_color(left_child(sibling), color_t::black);
					rotate_right(parent);
					i = m_Root;
				}
			}
		}

		set_color(i, color_t::black);
	}

	template <typename T, typename I, typename L, typename M>
	void CUtlRBTree<T, I, L, M>::unlink(std::size_t i) noexcept
	{
		std::size_t x;
		std::size_t y;

		if((left_child(i) == M::npos) ||
			(right_child(i) == M::npos))
		{
			y = i;
		} else {
			y = right_child(i);
			while(left_child(y) != M::npos) {
				y = left_child(y);
			}
		}

		if(left_child(y) != M::npos) {
			x = left_child(y);
		} else {
			x = right_child(y);
		}

		if(x != M::npos) {
			set_parent(x, child_parent(y));
		}

		if(!is_root(y)) {
			if(is_left_child(y)) {
				set_left_child(child_parent(y), x);
			} else {
				set_right_child(child_parent(y), x);
			}
		} else {
			set_root(x);
		}

		color_t ycolor{color(y)};
		if(y != i) {
			set_parent(y, child_parent(i));
			set_right_child(y, right_child(i));
			set_left_child(y, left_child(i));

			if(!is_root(i)) {
				if(is_left_child(i)) {
					set_left_child(child_parent(i), y);
				} else {
					set_right_child(child_parent(i), y);
				}
			} else {
				set_root(y);
			}

			if(left_child(y) != M::npos) {
				set_parent(left_child(y), y);
			}

			if(right_child(y) != M::npos) {
				set_parent(right_child(y), y);
			}

			set_color(y, color(i));
		}

		if((x != M::npos) && (ycolor == color_t::black)) {
			remove_rebalance(x);
		}
	}

	template <typename T, typename I, typename L, typename M>
	void CUtlRBTree<T, I, L, M>::free_node(std::size_t i) noexcept
	{
		m_Elements[i].m_Data.~T();
		set_left_child(i, i);
		set_right_child(i, m_FirstFree);
		m_FirstFree = static_cast<I>(i);
	}

	template <typename T, typename I, typename L, typename M>
	void CUtlRBTree<T, I, L, M>::erase(std::size_t i) noexcept
	{
		unlink(i);
		free_node(i);
		--m_NumElements;
	}

	template <typename T, typename I, typename L, typename M>
	std::size_t CUtlRBTree<T, I, L, M>::find(const key_type &key) const noexcept
	{
		T tmp;
		tmp.key = key;

		std::size_t current{m_Root};
		while(current != M::npos) {
			if(m_LessFunc(tmp, element(current))) {
				current = left_child(current);
			} else if(m_LessFunc(element(current), tmp)) {
				current = right_child(current);
			} else {
				break;
			}
		}

		return current;
	}
}

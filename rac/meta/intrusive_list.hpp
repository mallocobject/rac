#ifndef RAC_META_INTRUSIVE_LIST_HPP
#define RAC_META_INTRUSIVE_LIST_HPP

#include <cstddef>
namespace rac
{
template <typename T> struct Intrusive_list
{
	struct Node
	{
		using List = Intrusive_list<T>;
		Node() noexcept : list(nullptr), prev(nullptr), next(nullptr)
		{
		}

		~Node() noexcept
		{
			if (list)
			{
				list->eraseImpl(this);
			}
		}

		friend struct Intrusive_list;

	  private:
		List* list;
		Node* prev;
		Node* next;
	};

	Intrusive_list()
	{
		sentinel = new Node();

		sentinel->next = sentinel;
		sentinel->prev = sentinel;
	}

	Intrusive_list(Intrusive_list&&) = delete;

	~Intrusive_list() noexcept
	{
		Node* cur = sentinel->next;
		while (cur != sentinel)
		{
			cur->list = nullptr;
			cur = cur->next;
		}
		delete sentinel;
	}

	void insert(T& data) noexcept
	{
		insertImpl(&static_cast<Node&>(data));
	}

	void erase(T& data) noexcept
	{
		eraseImpl(&static_cast<Node&>(data));
	}

	std::size_t size() const noexcept
	{
		return count;
	}

  private:
	void insertImpl(Node* cur);
	void eraseImpl(Node* cur);

  private:
	Node* sentinel;
	std::size_t count{0};
};

template <typename T> void Intrusive_list<T>::insertImpl(Node* cur)
{
	cur->list = this;

	cur->next = sentinel;
	cur->prev = sentinel->prev;

	sentinel->prev->next = cur;
	sentinel->prev = cur;

	count++;
}

template <typename T> void Intrusive_list<T>::eraseImpl(Node* cur)
{
	cur->list = nullptr;

	cur->prev->next = cur->next;
	cur->next->prev = cur->prev;

	count--;
}
} // namespace rac

#endif
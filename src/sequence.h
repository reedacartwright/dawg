#pragma once
#ifndef DAWG_SEQUENCE_H
#define DAWG_SEQUENCE_H

#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <iterator>

#ifdef _WIN32
typedef unsigned int uint32_t;
#else
#include <stdint.h>
#endif

namespace dawg {
// http://www.cs.princeton.edu/courses/archive/fall08/cos226/lectures/10BalancedTrees-2x2.pdf

template<class _T, class _W> class finger_tree;

namespace detail {

//if(_T and const _X) are same

template<class _T, class _X=_T>
class finger_tree_node_iterator
	: public std::iterator<std::bidirectional_iterator_tag, _T>
{
public:
	typedef finger_tree_node_iterator<_T,_X> self_type;
	// other_type exists to enable iterator -> const_iterator conversions
	typedef finger_tree_node_iterator<_X,_X> other_type;
	typedef _T node_type;
	typedef std::iterator<std::bidirectional_iterator_tag, node_type> base_type;

	typedef typename base_type::value_type value_type;
	typedef typename base_type::difference_type difference_type;
	typedef typename base_type::pointer pointer;
	typedef typename base_type::reference reference;
	typedef typename base_type::iterator_category iterator_category;

	finger_tree_node_iterator() : p_node(NULL) { }
	explicit finger_tree_node_iterator(pointer p) : p_node(p) { }

	finger_tree_node_iterator(const other_type &o) : p_node(o.p_node) { }

	reference operator*() const { return *p_node; }
	pointer operator->() const { return p_node;	}
	self_type& operator++() {
		if(p_node->right != NULL) {
			p_node = p_node->right;
			while(p_node->left != NULL)
				p_node = p_node->left;
		} else {
			pointer p_up = p_node->up;
			while(p_node == p_up->right) {
				p_node = p_up;
				p_up = p_node->up;
			}
			if(p_node->right != p_up)
				p_node = p_up;
		}
		return *this;
	}
	self_type operator++(int) {
		self_type t = *this;
		++(*this);
		return t;
	}
	self_type& operator--() {
		if(p_node->up->up == p_node && p_node->color == true) {
			p_node = p_node->right;
		} else if(p_node->left != NULL) {
			p_node = p_node->left;
			while(p_node->right != NULL)
				p_node = p_node->right;
		} else {
			pointer p_up = p_node->up;
			while(p_node == p_up->left) {
				p_node = p_up;
				p_up = p_node->up;
			}
			p_node = p_up;
		}
		return *this;
	}
	self_type operator--(int) {
		self_type t = *this;
		--(*this);
		return t;
	}
	bool operator==(const self_type& x) const {
		return p_node == x.p_node;
	}
	bool operator!=(const self_type& x) const {
		return p_node != x.p_node;
	}

protected:
	pointer p_node;
};

}; // namespace dawg::detail

class residue {
public:
	typedef float rate_type;
	typedef uint32_t data_type;

	enum {
		base_mask   =  0x3F, // 111111
		delete_mask =  0x40, // 1000
		branch_mask = ~0x7F,
		delete_del	=  0x40,
		delete_ext  =  0x0,
		branch_inc  =  0x80
	};

	inline data_type base() const { return _data & base_mask; }
	inline void base(data_type b) { _data = (b & base_mask) | (_data & ~base_mask); }

	inline data_type branch() const { return _data & branch_mask; }
	inline void branch(data_type u) { _data = (u & branch_mask) | (_data & ~branch_mask); }

	inline data_type length() const { return is_deleted() ? 0 : 1; }

	inline data_type data()  const { return _data; }
	inline void data(data_type d) { _data = d; }
	inline void data(data_type a, bool b, data_type d) {
		_data = (a & base_mask) | (b ? delete_del : delete_ext) | (d & branch_mask);
	}

	inline bool is_deleted() const { return (_data & delete_mask) == delete_del; }
	inline void mark_deleted(bool b=true) {
		_data = (_data & ~delete_mask) | (b ? delete_del : delete_ext);
	}
	inline bool is_branch(data_type u) const { return (branch() == (u & branch_mask)); }

	inline rate_type rate_scalar() const {return _rate_scalar;}
	inline void rate_scalar(rate_type s) {
		_rate_scalar = s;
	}
	inline rate_type rate_length() const { return is_deleted() ? 0.0 : rate_scalar(); }

	residue() : _data(0), _rate_scalar(1.0) { }
	residue(data_type xbase, rate_type xscale, data_type xbranch, bool del=false) :
		_data((xbase & base_mask) | (xbranch & branch_mask) | (del ? delete_del : delete_ext) ),
		_rate_scalar(xscale)
	{

	}

protected:
	data_type  _data;
	rate_type  _rate_scalar;
};

template<class _D=float, class _N=uint32_t>
struct evo_node_weigher {
	typedef _D rate_type;
	typedef _N size_type;

	struct weight {
		typedef _D rate_type;
		typedef _N size_type;
		typedef typename evo_node_weigher<_D,_N>::weight self_type;

		rate_type rate;
		size_type length;

		weight() : rate(0), length(0) { }
		weight(const rate_type &r, const size_type &s) :
			rate(r), length(s) { }

		self_type operator+(const self_type &r) {
			return self_type(rate+r.rate, length+r.length);
		}

		self_type& operator+=(const self_type &r) {
			rate += r.rate;
			length += r.length;
			return *this;
		}

		self_type& operator-=(const self_type &r) {
			rate -= r.rate;
			length -= r.length;
			return *this;
		}

		bool operator<(const weight &r) const {
			return length < r.length;
		}

		operator rate_type() const {
			return rate;
		}
		operator size_type() const {
			return length;
		}
	};

	typedef weight weight_type;

	weight_type operator()(const residue& r) {
		return weight(base_rates[r.base()]*r.rate_length(),r.length());
	}
	rate_type *base_rates;
	evo_node_weigher() : base_rates(NULL) { }
	//evo_node_weigher(const evo_node_weigher & w) : base_rates(w.base_rates) { }
};

template<class _T, class _W>
class finger_tree {
public:
	struct node;
	typedef finger_tree<_T, _W> self_type;
	typedef _T data_type;
	typedef _W weigher_type;
	typedef typename weigher_type::weight_type weight_type;
	typedef typename std::size_t size_type;

	typedef detail::finger_tree_node_iterator<typename self_type::node> iterator;
	typedef detail::finger_tree_node_iterator<const typename self_type::node, typename self_type::node> const_iterator;
	typedef typename node::reference reference;
	typedef typename node::const_reference const_reference;
	typedef typename node::pointer pointer;
	typedef typename node::const_pointer const_pointer;

	finger_tree() : head(), _size(0) {
		head.up = &head;
		head.left = &head;
		head.right = &head;
	}
	~finger_tree() {
		if(head.up != &head)
			delete head.up;
		head.left = NULL;
		head.right = NULL;
	}

	finger_tree(const finger_tree &tree) : head(), _size(tree._size), _weigher(tree._weigher) {
		// tree is empty
		if(tree.head.up == &tree.head) {
			head.up = &head;
			head.left = &head;
			head.right = &head;
			return;
		}
		clone_head(tree.head);
	}

	finger_tree& operator=(const finger_tree &tree) {
		if(&tree == this)
			return *this;
		clear();
		_size = tree._size;
		_weigher = tree._weigher;
		if(tree.head.up == &tree.head) {
			head.up = &head;
			head.left = &head;
			head.right = &head;
			return *this;
		}
		clone_head(tree.head);
		return *this;
	}

	void swap(finger_tree &tree) {
		std::swap(_size, tree._size);
		std::swap(_weigher, tree._weigher);
		std::swap(head.up, tree.head.up);
		std::swap(head.right, tree.head.right);
		std::swap(head.left, tree.head.left);

		if(tree.head.up == &head) {
			tree.head.up = &tree.head;
			tree.head.left = &tree.head;
			tree.head.right = &tree.head;
		} else {
			tree.head.up->up = &tree.head;
		}
		if(head.up == &tree.head) {
			head.up = &head;
			head.left = &head;
			head.right = &head;
		} else {
			head.up->up = &head;
		}
	}

	void clear() {
		if(head.up == &head)
			return;
		delete head.up;
		head.up = &head;
		head.left = &head;
		head.right = &head;
		_size = 0;
	}

	iterator begin() {
		return iterator(head.left);
	}
	iterator end() {
		return iterator(&head);
	}
	iterator root() {
		return iterator(head.up);
	}
	const_iterator begin() const {
		return const_iterator(head.left);
	}
	const_iterator end() const {
		return const_iterator(&head);
	}
	const_iterator root() const {
		return const_iterator(head.up);
	}

	inline size_type size() const { return _size; }

	inline const weigher_type& weigher() const {
		return _weigher;
	}
	inline weigher_type& weigher() {
		return _weigher;
	}

	struct node {
		typedef node* pointer;
		typedef const node* const_pointer;
		typedef node& reference;
		typedef const node& const_reference;

		pointer left, right, up;
		bool color; // red == true; black == false
		data_type val;
		weight_type weight;

		node() : left(NULL), right(NULL), up(NULL), color(true),
			val(), weight() { }
		node(const data_type &v, const weight_type &w, pointer par=NULL) : left(NULL), right(NULL),
			up(par), color(true), val(v), weight(w) { }
		node(const node &n, pointer par) : left(NULL), right(NULL), up(par),
			color(n.color), val(n.val), weight(n.weight) { }
		~node() {
			if(left != NULL)
				delete left;
			if(right != NULL)
				delete right;
		}

		operator data_type() { return val; }
		operator data_type() const { return val; }

		private:
			node & operator=(const node& n);
	};

	// insert value before position in tree
	iterator insert(iterator pos, const data_type &val) {
		pointer x = new node(val, _weigher(val));
		rebalance(attach_left(&(*pos),x));
		head.up->color = false;
		++_size;
		return iterator(x);
	}

	template<class It>
	iterator insert(iterator pos, It it_begin, It it_end) {
		if(it_begin == it_end)
			return pos;

		pointer p = &(*pos);
		pointer x = new node(*it_begin, _weigher(*it_begin));
		rebalance(attach_left(p,x));
		++_size;
		while(++it_begin != it_end) {
			rebalance(attach_left(p,new node(*it_begin, _weigher(*it_begin))));
			++_size;
		}
		head.up->color = false;
		return iterator(x);
	}

	iterator insert(iterator pos, const self_type &tree) {
		insert(pos, tree.begin(), tree.end());
	}

//	void push_back(const data_type &val) {
//		insert(end(),val);
//	}

//	void push_front(const data_type &val) {
//		insert(begin(),val);
//	}

	//template<class _P>
	//data_type operator[](const _P &pos) {
	//	return find<_P>(pos)->val;
	//}

	//template<class _P>
	//data_type operator[](const _P &pos) const {
	//	return find<_P>(pos)->val;
	//}

	template<class _P>
	const_iterator find(const _P &pos) const {
		return const_iterator(find_node<_P>(pos));
	}

	template<class _P>
	iterator find(const _P &pos) {
		return iterator(const_cast<pointer>(find_node<_P>(pos)));
	}

	template<class _P>
	const_iterator search(const_iterator start, const _P &off) const {
		return const_iterator(inc_node(&(*start), off));
	}

	template<class _P>
	iterator search(iterator start, const _P &off) {
		return iterator(const_cast<pointer>(inc_node(&(*start), off)));
	}

	template<class _P>
	iterator update_and_search(iterator start, const _P &off) {
		return iterator(update_and_inc_node(&(*start), off));
	}

	iterator update_and_inc(iterator it) {
		pointer p = &*it;
		if(p->right != NULL) {
			p = p->right;
			while(p->left != NULL)
				p = p->left;
		} else {
			pointer pup = p->up;
			while(p == pup->right) {
				_update_weight(p);
				p = pup;
				pup = p->up;
			}
			if(p->right != pup) {
				_update_weight(p);
				p = pup;
			}
		}
		return iterator(p);
	}

	// update weight and propogate; should be called after
	// any modification of p->val that affects the weight
	void update_weight(iterator it) {
		pointer p = &*it;
		for(; p != &head; p=p->up)
			_update_weight(p);
	}

protected:
	inline bool is_null(pointer p) const {
		return (p == NULL);
	}
	inline bool is_red(pointer p) const {
		return (!is_null(p) && p->color == true);
	}

	// Attaches the node pointed to by x to just the left of p.
	// Returns the parent of the newly attached node
	pointer attach_left(pointer p, pointer x) {
		if(head.left == &head) {
			head.left = x;
			head.right = x;
			head.up = x;
			x->color = false;
		} else if(is_null(p->left)) {
			p->left = x;
			if(p == head.left)
				head.left = x;
		} else {
			if(p == &head)
				p = head.right;
			else for(p = p->left; !is_null(p->right); p = p->right)
				/*noop*/;
			p->right = x;
			if(p == head.right)
				head.right = x;
		}
		x->up = p;
		return p;
	}

	pointer rotate_left(pointer p) {
		pointer x = p->right;
		p->right = x->left;
		x->left = p;
		x->color = p->color;
		p->color = true;
		if(p->up == &head)
			head.up = x;
		else if(p->up->left == p)
			p->up->left = x;
		else
			p->up->right = x;
		x->up = p->up;
		p->up = x;
		if(p->right != NULL)
			p->right->up = p;
		_update_weight(p);
		return x;
	}

	pointer rotate_right(pointer p) {
		pointer x = p->left;
		p->left = x->right;
		x->right = p;
		x->color = p->color;
		p->color = true;
		if(p->up == &head)
			head.up = x;
		else if(p->up->left == p)
			p->up->left = x;
		else
			p->up->right = x;
		x->up = p->up;
		p->up = x;
		if(p->left != NULL)
			p->left->up = p;
		_update_weight(p);
		return x;
	}

	void flip_colors(pointer p) {
		p->color = true;
		p->left->color = false;
		p->right->color = false;
	}

	// recalculates the weight of this node
	void _update_weight(pointer p) {
		p->weight = _weigher(p->val);
		if(p->left != NULL)
			p->weight += p->left->weight;
		if(p->right != NULL)
			p->weight += p->right->weight;
	}

	// TODO: Limit _update_weight calls?
	void rebalance(pointer p, pointer top=NULL) {
		if(top == NULL)
			top = &head;
		for(; p != top; p = p->up) {
			if(is_red(p->right) && !is_red(p->left))
				p = rotate_left(p);
			if(is_red(p->left) && is_red(p->left->left))
				p = rotate_right(p);
			if(is_red(p->left) && is_red(p->right))
				flip_colors(p);
			_update_weight(p);
		}
	}

	// This will clone the structure and data of a head node
	// pointed to by p, to the head node of this tree.
	// Assumes that this tree is empty, and p is not.
	void clone_head(const node &ohead) {
		pointer p = ohead.up;
		head.up = new node(*p, &head);
		pointer q = head.up;
		while(p != &ohead) {
			if(p->left != NULL && q->left == NULL) {
				q->left = new node(*p->left, q);
				p = p->left;
				q = q->left;
			} else if(p->right != NULL && q->right == NULL) {
				q->right = new node(*p->right, q);
				p = p->right;
				q = q->right;
			} else {
				if(p == ohead.left)
					head.left = q;
				if(p == ohead.right)
					head.right = q;
				p = p->up;
				q = q->up;
			}
		}
	}

	template<class _P>
	const_pointer find_node(const _P &pos, const_pointer p=NULL) const {
		if(p == NULL)
			p = head.up;
        typedef _P cmp_type;
		cmp_type temp = pos;
		cmp_type pw = cmp_type(p->weight);
		if(!(temp < pw)) {
			return &head;
		}
		for(;;) {
			cmp_type cw;
			if(p->left != NULL) {
				cw = cmp_type(p->left->weight);
			 	if(temp < cw) {
					p = p->left;
					pw = cw;
					continue;
				}
			}
			if(p->right != NULL) {
				cw = cmp_type(p->right->weight);
				pw -= cw;
				if(!(temp < pw)) {
					temp -= pw;
					pw = cw;
					p = p->right;
					continue;
				}
			}
			break;
		}
		return p;
	}

	template<class _P>
	const_pointer inc_node(const_pointer p, const _P &pos) const {
		typedef _P cmp_type;
		cmp_type v = pos;
		for(;;) {
			if(p == &head)
				break;
			cmp_type w = cmp_type(p->weight);
			if(p->left != NULL)
				w -= cmp_type(p->left->weight);
			if(p->right != NULL) {
				cmp_type rw = cmp_type(p->right->weight);
				w -= rw;
				if(v < w)
					break;
				v -= w;
				if(v < rw)
					return find_node(v, p->right);
				v -= rw;
			} else {
				if(v < w)
					break;
				v -= w;
			}

			if(p->up->left == p) {
				p = p->up;
				continue;
			}
			for(p = p->up;p->up->right == p; p = p->up)
				/*noop*/;
			p = p->up;
		}
		return p;
	}

	template<class _P>
	pointer update_and_inc_node(pointer p, const _P &pos) {
		typedef _P cmp_type;
		cmp_type v = pos;
		for(;;) {
			if(p == &head)
				break;
			_update_weight(p);

			cmp_type w = cmp_type(p->weight);
			if(p->left != NULL)
				w -= cmp_type(p->left->weight);
			if(p->right != NULL) {
				cmp_type rw = cmp_type(p->right->weight);
				w -= rw;
				if(v < w)
					break;
				v -= w;
				if(v < rw)
					return const_cast<pointer>(find_node(v, p->right));
				v -= rw;
			} else {
				if(v < w)
					break;
				v -= w;
			}
			if(p->up->left == p) {
				p = p->up;
				continue;
			}
			do _update_weight(p = p->up); while(p->up->right == p);
			p = p->up;
		}
		return p;
	}


	node head;
	size_type _size;
	weigher_type _weigher;
};

class residue_factory {
public:
	typedef unsigned int model_type;
	typedef residue_factory self_type;

	static const model_type DNA = 0;
	static const model_type RNA = 1;
	static const model_type AA  = 2;
	static const model_type CODON = 3;

	template<class It, class D>
	void operator()(It b, It e, D &dest) {
		std::transform(b, e, std::back_inserter(dest),
			std::bind1st(std::mem_fun(&self_type::make_residue), this));
	}

	template<class T>
	residue operator()(T a) {
		return make_residue(a);
	}

	struct biop : public std::binary_function<char, double, residue> {
		biop(const residue_factory *o) : obj(o) { }
		const residue_factory *obj;
		residue operator()(char ch, double d) {
			return obj->make_residue(ch,d);
		}
	};
	/*
	template<typename It1, typename It2>
	seq_iterator operator()(It1 b, It1 e, It2 r) {
		store.push_back(Block());
		store.back().reserve(e-b);
		std::transform(b, e, r, std::back_inserter(store.back()), biop(this));
		return store.back().begin();
	}
	*/

	inline void model(model_type a) {_model = a; };

	residue make_residue(char ch) const {
		return residue(encode(ch), 1.0, 0);
	}
	residue make_residue(char ch, double d) const {
		return residue(encode(ch), static_cast<residue::rate_type>(d), 0);
	}

	residue::data_type encode(char ch) const {
		static residue::data_type dna[] = {0,1,3,2};
		if(_model == DNA || _model == RNA)
			return dna[(ch&6u) >> 1];
		return ~0;
	}
	char decode(residue::data_type r) const {
		static char dna[] = "ACGT";
		static char rna[] = "ACGU";
		// Include O & U, two rare amino acids
		static char aa[] = "ACDEFGHIKLMNPQRSTVWYOU";
		// Encode 64 possible codons using a base-64 notation
		// b/c this function returns a single char.
		// These can later be converted to three nucleotides or 1 amino acid
		static char cod[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.";

		switch(_model) {
		case DNA:
			return dna[r];
		case RNA:
			return rna[r];
		case AA:
			return aa[r];
		case CODON:
			return cod[r];
		default:
			break;
		}
		return '?';
	}

	residue_factory() : _model(DNA) { }

protected:
	model_type _model;
};

}; // namespace dawg


#endif


#include "range.hpp"
/*
 * Range-based storage device.
 */
template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Range<offset_t, enable_history, val_size_t>::Range(offset_t max_offset, val_size_t default_value)
	requires(enable_history): components::storage::Base<offset_t, enable_history, val_size_t>(max_offset),
	_default(default_value), _storage(),
	_delta(std::make_unique<components::delta::Vector<offset_t, val_size_t>>(*this))
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Range<offset_t, enable_history, val_size_t>::Range(offset_t max_offset, val_size_t default_value)
	requires(!enable_history): components::storage::Base<offset_t, enable_history, val_size_t>(max_offset),
	_default(default_value), _storage(), _delta(nullptr)
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Range<offset_t, enable_history, val_size_t>::clear(val_size_t fill_val)
{
	_storage.clear();
	_default = fill_val;

	if constexpr(enable_history) {
		auto ret = clear_delta();
		// If there's an error because there's no deltas enabled, we don't care.
		if(ret.has_error() && ret.error() != status_code(StorageErrc::DeltaDisabled)) {
			// Unrecoverable delta issue, kill application.
			ret.value();
		}
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::Range<offset_t, enable_history, val_size_t>::get(offset_t offset) const
{
	static auto comp = [](const components::storage::storage_span<offset_t>& lhs, offset_t rhs){
		return std::get<0>(lhs.span) < rhs;
	};

	if(offset > this->_max_offset) return oob_read_helper(offset);
	else {
		for(auto& span : _storage) {
			if (auto dist = offset - std::get<0>(span.span);
				dist <= std::get<1>(span.span) - std::get<0>(span.span)) {
					return span.value[dist];
			}
		}
	}
	return _default;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Range<offset_t, enable_history, val_size_t>::set(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) return oob_write_helper(offset, value);

	bool inserted = false, check_merge = false;
	// Attempt to modify an existing delta in-place if possible.
	for(auto& span : _storage) {
		if (auto dist = offset - std::get<0>(span.span);
			dist <= std::get<1>(span.span) - std::get<0>(span.span)) {
				span.value[dist] = value;
				inserted = true;
		}
		// Only extend uperwards, depend on merg step to combine downward.
		else if(offset == std::get<1>(span.span) + 1) {
			span.span = {std::get<0>(span.span), offset};
			span.value.push_back(value);
			inserted = true;
			check_merge = true;
		}
	}
	// Couldn't find an exisiting delta to modify, so must create new delta.
	if(!inserted) {
		_storage.push_back({{offset, offset}, {value}});
		check_merge = true;
	}
	// Check if we can merge any two deltas into a single delta.
	// Since we apply this step anytime our data changes, there is at most one change,
	// which means ranges may now be connected, but no range will overlap (i.e. have 
	// duplicate data elements).
	if(check_merge) {
		
		using _t = components::storage::storage_span<offset_t>;
		// Will compare on first key of tuple. Sufficient behavior because there should be no overlap between segments.
		auto compare = [](_t lhs, _t rhs){return lhs.span<rhs.span;};
		std::sort(_storage.begin(), _storage.end(), compare);

		// Allow endpoint to be re-computed as we remove elements
		for(int it=0; it<_storage.size()-1; it++) {
			// Since our deltas are sorted, only next could be adjacent to
			// current on current's right/upper bound. Since we iterate from 0>0xffff,
			// no need to compare LHS.
			auto cur = _storage[it];
			auto next = _storage[it+1];
			// Delta only borders in upperbound and lower bound are adjacent integers.
			if(std::get<1>(cur.span)+1 == std::get<0>(next.span)) {
				// Merge current+next into current.
				cur.span = {std::get<0>(cur.span), std::get<1>(next.span)};
				cur.value.insert(cur.value.end(), next.value.begin(), next.value.end());
				// Remove next delta, since it has been merged in.
				_storage.erase(_storage.begin()+it+1);
			}
		}
	}
	return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::Range<offset_t, enable_history, val_size_t>::read(offset_t offset) const
{
	// No need to perform any delta computation, as reading never changes the state of non-memory-mapped storage.
	return get(offset);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Range<offset_t, enable_history, val_size_t>::write(offset_t offset, val_size_t value)
{
	if constexpr(enable_history) {
		// This is a redundant check with set(), but it is very important that we don't generate illegal deltas.
		if(offset > this->_max_offset) return oob_write_helper(offset, value);
		_delta->add_delta(offset, get(offset).value(), value);
	}
	return set(offset, value);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
bool components::storage::Range<offset_t, enable_history, val_size_t>::deltas_enabled() const
{
	return enable_history;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Range<offset_t, enable_history, val_size_t>::clear_delta()
{	
	if constexpr(enable_history) {
		_delta->clear();
		return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> components::storage::Range<offset_t, enable_history, val_size_t>::take_delta()
{	
	if constexpr(enable_history) {
		// Helper for enabling std::swap.
		using std::swap;
		auto ret = std::make_unique<components::delta::Vector<offset_t, val_size_t>>(*this);
		swap(ret, _delta);
		return {std::move(ret)};
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Range<offset_t, enable_history, val_size_t>::resize(offset_t new_offset) 
{
	this->_max_offset = new_offset;
	clear();
	return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}
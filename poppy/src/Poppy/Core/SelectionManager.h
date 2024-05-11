#ifndef POPPY_CORE_SELECTIONMANAGER_H
#define POPPY_CORE_SELECTIONMANAGER_H

#include <optional>
#include <span>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>

namespace poppy {

/// Different modes in which selections can be amended
enum class SelectionMode {
    Replace, /// Default, existing selection is replaced
    Add,     /// Additive selection, new selection does not unselect existing
             /// selection
    Toggle   /// "Exclusive-or" selection. Items are selected iff not already
             /// selected
};

/// Determines the current selection mode based on held modifier keys. Must be
/// called within an ImGui frame
SelectionMode getSelectionMode();

/// Generic selection manager that supports replace-, add-, toggle- and multiple
/// seletion
template <typename T>
class BasicSelectionManager {
public:
    /// \Returns `true` if \p elem is selected
    template <typename U>
    bool contains(U const& elem) const {
        if (!_candidateMode) {
            return _elems.contains(elem);
        }
        switch (*_candidateMode) {
        case SelectionMode::Replace:
            return _candidates.contains(elem);
        case SelectionMode::Add:
            return _elems.contains(elem) || _candidates.contains(elem);
        case SelectionMode::Toggle:
            return _elems.contains(elem) != _candidates.contains(elem);
        }
    }

    /// Calls `set`, `add` or `toggle` depending on \p mode
    /// \Returns `true` if \p elem is selected after the call
    bool amend(T const& elem, SelectionMode mode) {
        switch (mode) {
        case SelectionMode::Replace:
            set(elem);
            return true;
        case SelectionMode::Add:
            add(elem);
            return true;
        case SelectionMode::Toggle:
            return toggle(elem);
        }
    }

    /// \Returns `amend(elem, getSelectionMode())`
    bool amend(T const& elem) { return amend(elem, getSelectionMode()); }

    /// Adds \p elem to the selection
    void add(T const& elem) { _elems.insert(elem); }

    /// \overload for ranges
    template <typename Itr, typename Sentinel>
    void add(Itr first, Sentinel last) {
        _elems.insert(first, last);
    }

    /// Erases \p elem from the selection
    /// \Returns `true` if the element was selected
    template <typename U>
    bool erase(U const& elem) {
        return _elems.erase(elem);
    }

    /// \overload for ranges
    void add(ranges::range auto&& rng) {
        add(ranges::begin(rng), ranges::end(rng));
    }

    /// If \p elem is not selected, adds \p elem to the selection. Otherwise
    /// removes \p elem from the selection
    /// \Returns `true` if \p elem is selected after the call
    bool toggle(T const& elem) {
        auto itr = _elems.find(elem);
        if (itr != _elems.end()) {
            _elems.erase(itr);
            return false;
        }
        else {
            add(elem);
            return true;
        }
    }

    /// Clears the selection and selects \p elem
    void set(T const& elem) { set(&elem, &elem + 1); }

    /// \overload for ranges
    template <typename Itr, typename Sentinel>
    void set(Itr first, Sentinel last) {
        clear();
        add(first, last);
    }

    /// \overload for ranges
    void set(ranges::range auto&& rng)
        requires(!std::convertible_to<decltype(rng), T const&>)
    {
        set(ranges::begin(rng), ranges::end(rng));
    }

    /// Unselectes all elements
    void clear() { _elems.clear(); }

    /// \Returns a view over all selected elements
    std::span<T const> elements() const { return _elems.values(); }

    /// \Returns the number of selected elements
    size_t size() const { return _elems.size(); }

    /// \Returns `true` if no elements are selected
    bool empty() const { return _elems.empty(); }

    /// Begin multi selection with selection mode \p mode
    void beginCandidates(SelectionMode mode) { this->_candidateMode = mode; }

    /// Update multi selection
    void setCandidates(utl::hashset<T> elems) {
        _candidates = std::move(elems);
    }

    /// Finish multi selection
    void applyCandidates() {
        if (!_candidateMode) {
            return;
        }
        switch (*_candidateMode) {
        case SelectionMode::Replace:
            _elems = std::move(_candidates);
            break;
        case SelectionMode::Add:
            _elems.insert(_candidates.begin(), _candidates.end());
            _candidates.clear();
            break;
        case SelectionMode::Toggle: {
            for (auto const& elem: _candidates) {
                auto itr = _elems.find(elem);
                if (itr != _elems.end()) {
                    _elems.erase(itr);
                }
                else {
                    _elems.insert(elem);
                }
            }
            break;
        }
        }
        _candidateMode = std::nullopt;
    }

    /// Applies \p fn to all selected elements
    void transform(std::invocable<T const&> auto fn) {
        auto values = _elems.values();
        _elems.clear();
        for (auto& elem: values) {
            _elems.insert(std::invoke(fn, elem));
        }
    }

private:
    utl::hashset<T> _elems;
    utl::hashset<T> _candidates;
    std::optional<SelectionMode> _candidateMode;
};

} // namespace poppy

#endif // POPPY_CORE_SELECTIONMANAGER_H

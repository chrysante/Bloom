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
template <typename T,
          typename OptT =
              std::conditional_t<std::is_pointer_v<T>, T, std::optional<T>>,
          typename Hash = utl::hash<T>, typename KeyEq = std::equal_to<>>
class BasicSelectionManager {
public:
    using SetType = utl::hashset<T, Hash, KeyEq>;

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
    bool amend(T const& elem, SelectionMode mode = getSelectionMode()) {
        return amendImpl(mode, elem);
    }

    template <typename Itr, typename Sentinel>
    void amend(Itr first, Sentinel last,
               SelectionMode mode = getSelectionMode()) {
        amendImpl(mode, first, last);
    }

    void amend(ranges::range auto&& rng,
               SelectionMode mode = getSelectionMode())
        requires(!std::convertible_to<decltype(rng), T const&>)
    {
        amendImpl(mode, std::forward<decltype(rng)>(rng));
    }

    /// Adds \p elem to the selection
    void add(T const& elem) {
        _elems.insert(elem);
        _last = elem;
    }

    /// \overload for ranges
    template <typename Itr, typename Sentinel>
    void add(Itr first, Sentinel last) {
        _elems.insert(first, last);
        if (first != last) {
            _last = *std::prev(last);
        }
    }

    /// \overload for ranges
    void add(ranges::range auto&& rng)
        requires(!std::convertible_to<decltype(rng), T const&>)
    {
        add(ranges::begin(rng), ranges::end(rng));
    }

    /// If \p elem is not selected, adds \p elem to the selection. Otherwise
    /// removes \p elem from the selection
    /// \Returns `true` if \p elem is selected after the call
    bool toggle(T const& elem) {
        auto itr = _elems.find(elem);
        if (itr != _elems.end()) {
            _elems.erase(itr);
            _last = OptT{};
            return false;
        }
        else {
            add(elem);
            _last = elem;
            return true;
        }
    }

    template <typename Itr, typename Sentinel>
    void toggle(Itr first, Sentinel last) {
        for (; first != last; ++first) {
            toggle(*first);
        }
    }

    void toggle(ranges::range auto&& rng)
        requires(!std::convertible_to<decltype(rng), T const&>)
    {
        toggle(ranges::begin(rng), ranges::end(rng));
    }

    /// Clears the selection and selects \p elem
    void set(T const& elem) {
        set(&elem, &elem + 1);
        _last = elem;
    }

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

    /// Erases \p elem from the selection
    /// \Returns `true` if the element was selected
    template <typename U>
    bool erase(U const& elem) {
        if (_last == elem) {
            _last = std::nullopt;
        }
        return _elems.erase(elem);
    }

    /// Unselectes all elements
    void clear() {
        _elems.clear();
        _last = OptT{};
    }

    /// \Returns a view over all selected elements
    std::span<T const> elements() const { return _elems.values(); }

    auto begin() const { return elements().begin(); }

    auto end() const { return elements().end(); }

    /// \Returns the number of selected elements
    size_t size() const { return _elems.size(); }

    /// \Returns `true` if no elements are selected
    bool empty() const { return _elems.empty(); }

    /// Begin multi selection with selection mode \p mode
    void beginCandidates(SelectionMode mode) { this->_candidateMode = mode; }

    /// Update multi selection
    void setCandidates(SetType elems) { _candidates = std::move(elems); }

    /// Finish multi selection
    void applyCandidates() {
        if (!_candidateMode) {
            return;
        }
        amend(std::move(_candidates), *_candidateMode);
        _candidates.clear();
        _candidateMode = std::nullopt;
    }

    /// Applies \p fn to all selected elements
    void transform(std::invocable<T const&> auto fn) {
        auto values = _elems.values();
        _elems.clear();
        for (auto& elem: values) {
            _elems.insert(std::invoke(fn, elem));
        }
        if (_last) {
            _last = std::invoke(fn, *_last);
        }
    }

    /// \Returns the last selected element
    OptT last() const { return _last; }

    /// \Returns the single selected element or `OptT{}` or none or
    /// multiple elements are selected
    OptT single() const {
        return _elems.size() == 1 ? OptT(*_elems.begin()) : OptT{};
    }

private:
    template <typename... Args>
    bool amendImpl(SelectionMode mode, Args&&... args) {
        switch (mode) {
        case SelectionMode::Replace:
            set(std::forward<Args>(args)...);
            return true;
        case SelectionMode::Add:
            add(std::forward<Args>(args)...);
            return true;
        case SelectionMode::Toggle:
            if constexpr (std::is_same_v<decltype(toggle(
                                             std::forward<Args>(args)...)),
                                         void>)
            {
                toggle(std::forward<Args>(args)...);
                return bool{};
            }
            else {
                return toggle(std::forward<Args>(args)...);
            }
        }
    }

    SetType _elems;
    SetType _candidates;
    OptT _last = {};
    std::optional<SelectionMode> _candidateMode;
};

class TransactionManager;

template <typename T, typename OptT, typename Hash, typename KeyEq>
void editSelection(std::same_as<TransactionManager> auto& TAM,
                   BasicSelectionManager<T, OptT, Hash, KeyEq>& selection,
                   std::invocable auto fn) {
    auto commit = [fn] {
        std::invoke(fn);
        return true;
    };
    auto undo = [backup = selection, &selection] {
        selection = backup;
        return true;
    };
    TAM.commit(std::move(commit), std::move(undo));
}

} // namespace poppy

#endif // POPPY_CORE_SELECTIONMANAGER_H

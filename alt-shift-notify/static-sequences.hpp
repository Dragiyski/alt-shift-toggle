/*
 * Copyright (C) 2018 Plamen Dragiyski <plamen@dragiyski.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tuple>
#include <type_traits>

class MatchKey {
protected:
    bool m_match = false;
    bool m_fail = false;
public:
    using Key = decltype(std::declval<struct input_event>().code);
    virtual void handleEvent(Key keyCode, bool state) = 0;
    bool match() {
        return m_match;
    };
    bool fail() {
        return m_fail;
    };
};

class LockKey : public MatchKey {
    Key m_key;
public:
    explicit LockKey(Key key) : m_key(key) {
    }
};

class UnlockKey : public MatchKey {
    Key m_key;
public:
    explicit UnlockKey(Key key) : m_key(key) {
    }
};

template<typename ... Args>
class MatchGroup : public MatchKey {
    std::tuple<std::pair<Args, bool>...> m_state;
protected:
    constexpr bool handleEventState(Key keyCode, bool state) {
        return handleEventState(keyCode, state, std::make_index_sequence<sizeof...(Args)>{});
    }
    template<size_t ... I>
    constexpr bool handleEventState(Key keyCode, bool state, std::index_sequence<I...>) {
        return (handleEventState(keyCode, state, std::get<I>(m_state)) || ...);
    }
    template<typename keyCodeType>
    constexpr bool handleEventState(Key keyCode, bool state, std::pair<keyCodeType, bool>& stateMap) {
        if(keyCode == stateMap.first) {
            stateMap.second = state;
            return true;
        }
        return false;
    }
    constexpr bool allActive() const {
        return allActive(std::make_index_sequence<sizeof...(Args)>{});
    }
    template<size_t ... I>
    constexpr bool allActive(std::index_sequence<I...>) const {
        return (std::get<I>(m_state).second && ...);
    }
    constexpr bool anyActive() const {
        return anyActive(std::make_index_sequence<sizeof...(Args)>{});
    }
    template<size_t ... I>
    constexpr bool anyActive(std::index_sequence<I...>) const {
        return (std::get<I>(m_state).second && ...);
    }
public:
    MatchGroup() = delete;
    explicit MatchGroup(Args ... args) : m_state(std::make_tuple(std::make_pair(args, false)...)) {
        static_assert((std::is_convertible<decltype(args), Key>::value && ...), "Expected LockGroup parameters to be of type Key");
    }
};

template<typename ... Args>
class LockGroup final : public MatchGroup<Args...> {
public:
    using Key = typename MatchGroup<Args...>::Key;
public:
    LockGroup(Args ... args) : MatchGroup<Args...>(args...) {
    }
    void handleEvent(Key keyCode, bool state) {
        if(MatchGroup<Args...>::handleEventState(keyCode, state)) {
            if(MatchGroup<Args...>::allActive()) {
                MatchGroup<Args...>::m_match = true;
            }
        } else {
            MatchGroup<Args...>::m_fail = true;
        }
    }
};

template<typename ... Args>
class UnlockGroup final : public MatchGroup<Args...> {
public:
    using Key = typename MatchGroup<Args...>::Key;
public:
    UnlockGroup(Args ... args) : MatchGroup<Args...>(args...) {
    }
    void handleEvent(Key keyCode, bool state) {
        if(MatchGroup<Args...>::handleEventState(keyCode, state)) {
            if(!MatchGroup<Args...>::allActive()) {
                MatchGroup<Args...>::m_match = true;
            }
        } else {
            MatchGroup<Args...>::m_fail = true;
        }
    }
};

template<typename ... Args>
class ReleaseGroup final : public MatchGroup<Args...> {
public:
    using Key = typename MatchGroup<Args...>::Key;
public:
    ReleaseGroup(Args ... args) : MatchGroup<Args...>(args...) {
    }
    void handleEvent(Key keyCode, bool state) {
        if(MatchGroup<Args...>::handleEventState(keyCode, state)) {
            if(!MatchGroup<Args...>::allActive()) {
                MatchGroup<Args...>::m_match = true;
            }
        } else {
            MatchGroup<Args...>::m_fail = true;
        }
    }
};

template<typename ... Args>
class KeySequence {
    std::tuple<Args...> m_sequence;
public:
    KeySequence(Args ... args) : m_sequence(std::make_tuple(args...)) {
        static_assert((std::is_base_of<MatchKey, Args>::value && ...), "Expected KeySequence parameter to be derived from MatchKey");
    }
};

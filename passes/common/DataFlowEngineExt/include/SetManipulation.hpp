#pragma once

#include <set>
#include <vector>
#include <algorithm>

class SetManipulation {
public:
  template <typename T>
  static std::set<T> Intersection(std::set<T> &SetA, std::set<T> &SetB) {
    std::set<T> IntersectionSet;
    set_intersection(SetA.begin(), SetA.end(), SetB.begin(), SetB.end(),
                     inserter(IntersectionSet, IntersectionSet.begin()));
    return IntersectionSet;
  }

  template <typename T>
  static std::set<T> Intersection(std::vector<std::set<T>> &Sets) {
    auto SetSize = Sets.size();

    if (SetSize == 0) {
      return {};
    } else if (SetSize == 1) {
      return {Sets[0].begin(), Sets[0].end()};
    }

    // SetSize > 1
    std::set<T> TempA, TempB;
    std::set<T> *TempUsePtr = &TempA, *TempDestPtr = &TempB;

    *TempUsePtr = Intersection(Sets[0], Sets[1]);
    for (size_t i = 2; i < SetSize; i++) {
      *TempDestPtr = Intersection(*TempUsePtr, Sets[i]);

      // swap
      auto Temp = TempUsePtr;
      TempUsePtr = TempDestPtr;
      TempDestPtr = Temp;
    }
    return *TempUsePtr;
  }

  template <typename T>
  static std::set<T> Union(std::set<T> &SetA, std::set<T> &SetB) {
    std::set<T> UnionSet;
    UnionSet.insert(SetA.begin(), SetA.end());
    UnionSet.insert(SetB.begin(), SetB.end());
    return UnionSet;
  }

  template <typename T>
  static std::set<T> Union(std::vector<std::set<T>> &Sets) {
    std::set<T> UnionSet;
    for (const auto &S : Sets) {
      UnionSet.insert(S.begin(), S.end());
    }
    return UnionSet;
  }
};

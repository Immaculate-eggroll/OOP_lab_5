#include <iostream>
#include <string>
#include "pmr_slist.h"
#include "MapMemoryResource.h"

struct Person {
    std::string name;
    int age;

    Person(std::string n, int a) : name(std::move(n)), age(a) {}
};

int main() {
    MapMemoryResource mem(1024 * 1024);

    pmr_slist<int> nums(&mem);
    nums.push_front(10);
    nums.push_front(20);
    nums.push_front(30);

    for (auto& n : nums) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    pmr_slist<Person> people(&mem);

    people.push_front("Alice", 30);
    people.push_front("Bob", 25);

    for (auto& p : people) {
        std::cout << p.name << " " << p.age << std::endl;
    }

    return 0;
}

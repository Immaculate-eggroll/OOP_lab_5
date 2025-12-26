#include <gtest/gtest.h>
#include "pmr_slist.h"
#include "MapMemoryResource.h"

#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

TEST(MapMemoryResourceTest, BasicAllocationAndDeallocation) {
    constexpr std::size_t BUFFER_SIZE = 1024;
    MapMemoryResource mem(BUFFER_SIZE);
    
    void* ptr = mem.allocate(64, 8);
    EXPECT_NE(ptr, nullptr);
    
    EXPECT_NO_THROW(mem.deallocate(ptr, 64, 8));
}

TEST(MapMemoryResourceTest, AlignmentRequirements) {
    MapMemoryResource mem(1024);
    
    struct TestCase {
        std::size_t size;
        std::size_t alignment;
    };
    
    std::vector<TestCase> tests = {
        {1, 1},
        {16, 8},
        {32, 16},
        {64, 32},
        {128, 64}
    };
    
    for (const auto& tc : tests) {
        void* ptr = mem.allocate(tc.size, tc.alignment);
        EXPECT_NE(ptr, nullptr);
        
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
        EXPECT_EQ(addr % tc.alignment, 0U) 
            << "Failed alignment test: size=" << tc.size 
            << ", alignment=" << tc.alignment;
        
        mem.deallocate(ptr, tc.size, tc.alignment);
    }
}

TEST(MapMemoryResourceTest, MemoryReuse) {
    MapMemoryResource mem(256);
    
    // Выделяем и запоминаем адрес
    void* first = mem.allocate(64, 8);
    std::uintptr_t first_addr = reinterpret_cast<std::uintptr_t>(first);
    
    // Освобождаем
    mem.deallocate(first, 64, 8);
    
    // Выделяем снова - должен получить тот же блок
    void* second = mem.allocate(64, 8);
    std::uintptr_t second_addr = reinterpret_cast<std::uintptr_t>(second);
    
    EXPECT_EQ(first_addr, second_addr) << "Memory not reused";
    
    mem.deallocate(second, 64, 8);
}

TEST(MapMemoryResourceTest, OutOfMemoryThrows) {
    // Создаём очень маленький буфер
    MapMemoryResource mem(10);
    
    // Пытаемся выделить больше, чем есть
    // Используем ASSERT_THROW для немедленного прерывания теста при неудаче
    ASSERT_THROW({
        void* p = mem.allocate(20, 1);
        mem.deallocate(p, 20, 1);
    }, std::bad_alloc);
    
    void* ptr = mem.allocate(5, 1);
    EXPECT_NE(ptr, nullptr);
    mem.deallocate(ptr, 5, 1);
}

TEST(MapMemoryResourceTest, DifferentBlockSizes) {
    MapMemoryResource mem(2048);
    
    std::vector<std::pair<void*, std::size_t>> allocations;
    
    // Выделяем блоки разных размеров
    allocations.emplace_back(mem.allocate(1, 1), 1);      // 1 байт
    allocations.emplace_back(mem.allocate(16, 8), 16);    // 16 байт
    allocations.emplace_back(mem.allocate(256, 16), 256); // 256 байт
    allocations.emplace_back(mem.allocate(512, 32), 512); // 512 байт
    
    // Проверяем что все выделились
    for (const auto& [ptr, size] : allocations) {
        EXPECT_NE(ptr, nullptr) << "Failed to allocate " << size << " bytes";
    }
    
    // Освобождаем в обратном порядке
    for (auto it = allocations.rbegin(); it != allocations.rend(); ++it) {
        mem.deallocate(it->first, it->second, 8);
    }
}

TEST(MapMemoryResourceTest, IsEqualImplementation) {
    MapMemoryResource mem1(1024);
    MapMemoryResource mem2(1024);
    
    // Ресурс должен быть равен только самому себе
    EXPECT_TRUE(mem1.is_equal(mem1));
    EXPECT_TRUE(mem2.is_equal(mem2));
    EXPECT_FALSE(mem1.is_equal(mem2));
}

TEST(PmrSlistTest, DefaultConstruction) {
    pmr_slist<int> list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.begin(), list.end());
}

TEST(PmrSlistTest, PushFrontAndIteration) {
    pmr_slist<int> list;
    
    list.push_front(1);
    EXPECT_FALSE(list.empty());
    
    list.push_front(2);
    list.push_front(3);
    
    // Проверяем порядок (элементы добавляются в начало)
    std::vector<int> values;
    for (int val : list) {
        values.push_back(val);
    }
    
    EXPECT_EQ(values.size(), 3U);
    EXPECT_EQ(values[0], 3);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 1);
}

TEST(PmrSlistTest, PopFront) {
    pmr_slist<std::string> list;
    
    list.push_front("first");
    list.push_front("second");
    list.push_front("third");
    
    EXPECT_FALSE(list.empty());
    
    list.pop_front(); // Удаляем "third"
    
    // Проверяем оставшиеся элементы
    auto it = list.begin();
    EXPECT_EQ(*it, "second");
    ++it;
    EXPECT_EQ(*it, "first");
    ++it;
    EXPECT_EQ(it, list.end());
    
    list.pop_front(); // "second"
    list.pop_front(); // "first"
    
    EXPECT_TRUE(list.empty());
}

TEST(PmrSlistTest, Clear) {
    pmr_slist<int> list;
    
    for (int i = 0; i < 10; ++i) {
        list.push_front(i);
    }
    
    EXPECT_FALSE(list.empty());
    
    list.clear();
    
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.begin(), list.end());
}

TEST(PmrSlistTest, CustomMemoryResource) {
    MapMemoryResource mem(4096);
    pmr_slist<int> list(&mem);
    
    // Заполняем список
    for (int i = 0; i < 100; ++i) {
        list.push_front(i);
    }
    
    // Проверяем что все элементы на месте
    int count = 0;
    // Убираем sum, если не используется
    for ([[maybe_unused]] int val : list) {
        ++count;
    }
    
    EXPECT_EQ(count, 100);
    
    list.clear();
    EXPECT_TRUE(list.empty());
}

TEST(PmrSlistTest, MoveSemantics) {
    MapMemoryResource mem(1024);
    
    pmr_slist<int> source(&mem);
    source.push_front(10);
    source.push_front(20);
    source.push_front(30);
    
    // Move constructor
    pmr_slist<int> destination(std::move(source));
    
    EXPECT_TRUE(source.empty());
    EXPECT_FALSE(destination.empty());
    
    // Проверяем содержимое
    std::vector<int> values;
    for (int val : destination) {
        values.push_back(val);
    }
    
    EXPECT_EQ(values.size(), 3U);
    EXPECT_EQ(values[0], 30);
    EXPECT_EQ(values[1], 20);
    EXPECT_EQ(values[2], 10);
}

TEST(PmrSlistTest, ComplexTypes) {
    struct Person {
        std::string name;
        int age;
        
        Person(const std::string& n, int a) : name(n), age(a) {}
    };
    
    MapMemoryResource mem(2048);
    pmr_slist<Person> people(&mem);
    
    people.push_front("Alice", 30);
    people.push_front("Bob", 25);
    people.push_front("Charlie", 35);
    
    // Проверяем через итераторы
    auto it = people.begin();
    EXPECT_EQ((*it).name, "Charlie");
    EXPECT_EQ((*it).age, 35);
    
    ++it;
    EXPECT_EQ((*it).name, "Bob");
    EXPECT_EQ((*it).age, 25);
    
    ++it;
    EXPECT_EQ((*it).name, "Alice");
    EXPECT_EQ((*it).age, 30);
    
    ++it;
    EXPECT_EQ(it, people.end());
}

TEST(PmrSlistTest, StressTest) {
    constexpr int NUM_ELEMENTS = 500; // Уменьшаем для надёжности
    MapMemoryResource mem(NUM_ELEMENTS * 128); // Достаточный запас
    
    pmr_slist<int> list(&mem);
    
    // Добавляем много элементов
    for (int i = 0; i < NUM_ELEMENTS; ++i) {
        list.push_front(i);
    }
    
    // Проверяем количество
    int count = 0;
    for ([[maybe_unused]] int val : list) {
        ++count;
    }
    EXPECT_EQ(count, NUM_ELEMENTS);
    
    list.clear();
    EXPECT_TRUE(list.empty());
}

TEST(PmrSlistTest, EmptyListOperations) {
    pmr_slist<int> list;
    
    // Операции на пустом списке
    EXPECT_NO_THROW(list.pop_front()); // Должно ничего не делать
    
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.begin(), list.end());
    
    // Добавляем и удаляем
    list.push_front(42);
    EXPECT_FALSE(list.empty());
    
    list.pop_front();
    EXPECT_TRUE(list.empty());
}


TEST(IntegrationTest, SlistWithMapResource) {
    // Тестируем полный цикл работы
    MapMemoryResource mem(4096);  // Увеличиваем буфер
    
    pmr_slist<std::string> list(&mem);
    
    // Добавляем строки разной длины
    list.push_front("short");
    list.push_front("medium string");
    list.push_front("long string");
    
    // Итерируем и проверяем
    std::vector<std::string> collected;
    for (const auto& str : list) {
        collected.push_back(str);
    }
    
    EXPECT_EQ(collected.size(), 3U);
    EXPECT_EQ(collected[0], "long string");
    EXPECT_EQ(collected[1], "medium string");
    EXPECT_EQ(collected[2], "short");
    
    // Очищаем
    list.clear();
    
    // Память должна быть доступна для повторного использования
    pmr_slist<int> new_list(&mem);
    for (int i = 0; i < 50; ++i) {
        new_list.push_front(i);
    }
    EXPECT_FALSE(new_list.empty());
}

TEST(IntegrationTest, ResourceExhaustionAndRecovery) {
    // Тест на исчерпание ресурса и восстановление
    constexpr std::size_t SMALL_BUFFER = 256;
    MapMemoryResource mem(SMALL_BUFFER);
    
    pmr_slist<int> list(&mem);
    
    // Пытаемся заполнить до исчерпания
    bool exception_thrown = false;
    try {
        for (int i = 0; i < 1000; ++i) {
            list.push_front(i);
        }
    } catch (const std::bad_alloc&) {
        exception_thrown = true;
    }
    
    // Ожидаем исключение из-за нехватки памяти
    EXPECT_TRUE(exception_thrown);
    
    // Очищаем и пробуем снова
    list.clear();
    
    // Теперь должно работать
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(list.push_front(i));
    }
    
    EXPECT_FALSE(list.empty());
}

TEST(PmrSlistTest, IteratorOperations) {
    pmr_slist<int> list;
    
    list.push_front(3);
    list.push_front(2);
    list.push_front(1);
    
    auto it1 = list.begin();
    auto it2 = list.begin();
    
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(*it1, 1);
    
    ++it1;
    EXPECT_NE(it1, it2);
    EXPECT_EQ(*it1, 2);
    
    ++it1;
    EXPECT_EQ(*it1, 3);
    
    ++it1;
    EXPECT_EQ(it1, list.end());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

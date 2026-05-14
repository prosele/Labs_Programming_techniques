#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <random>
#include <filesystem>

using namespace std;

// Первая часть названия услуги
const vector<string> PREFIXES = {
    "Разработка", "Проектирование", "Внедрение", "Интеграция",
    "Тестирование", "Аудит", "Сопровождение", "Миграция",
    "Оптимизация", "Настройка", "Развёртывание", "Консультация"
};
// Вторая часть названия услуги
const vector<string> OBJECTS = {
    "веб-приложения", "мобильного приложения", "CRM-системы",
    "ERP-системы", "API-сервиса", "базы данных", "микросервисной архитектуры",
    "облачной инфраструктуры", "корпоративного портала", "системы мониторинга",
    "платёжного модуля", "системы аутентификации", "чат-бота",
    "панели администратора", "системы аналитики", "DevOps-пайплайна",
    "сети доставки контента", "системы резервного копирования",
    "нагрузочного тестирования", "системы управления задачами"
};
// Третья часть названия услуги
const vector<string> SUFFIXES = {
    "", " (MVP)", " (Enterprise)", " (SaaS)", " под ключ",
    " с нуля", " на основе существующего решения", " (POC)"
};
// Диапазоны стоимостей по типу услуги (руб.)
struct PriceRange { int min, max; };
const vector<PriceRange> PRICE_RANGES = {
    {50000,  2000000},  // Разработка
    {30000,   800000},  // Проектирование
    {80000,  1500000},  // Внедрение
    {60000,  1200000},  // Интеграция
    {20000,   400000},  // Тестирование
    {15000,   300000},  // Аудит
    {10000,   150000},  // Сопровождение (в мес.)
    {40000,   900000},  // Миграция
    {25000,   500000},  // Оптимизация
    {10000,   200000},  // Настройка
    {30000,   600000},  // Развёртывание
    {5000,     80000},  // Консультация
};
// Структура услуги
struct ITService {
    string name;
    double cost;        // стоимость, руб.
    int    duration;    // срок исполнения, дней
    double prepayment;  // предоплата, руб.
};
// Генератор
class ServiceGenerator {
    mt19937 rng;
    int randInt(int lo, int hi) {
        return uniform_int_distribution<int>(lo, hi)(rng);
    }
    double randDouble(double lo, double hi) {
        return uniform_real_distribution<double>(lo, hi)(rng);
    }
public:
    explicit ServiceGenerator(unsigned seed = 42) : rng(seed) {}
    ITService generate() {
        ITService s;
        // Название: Prefix + Object + Suffix
        int pi = randInt(0, (int)PREFIXES.size() - 1);
        int oi = randInt(0, (int)OBJECTS.size() - 1);
        int si = randInt(0, (int)SUFFIXES.size() - 1);
        s.name = PREFIXES[pi] + " " + OBJECTS[oi] + SUFFIXES[si];

        // Стоимость из диапазона, характерного для типа услуги
        auto [minP, maxP] = PRICE_RANGES[pi];
        // Округляем до 500 руб. — выглядит реалистично
        double raw = randDouble(minP, maxP);
        s.cost = round(raw / 500.0) * 500.0;

        // Срок: примерно 1 день на каждые 10 000 руб., плюс случайный разброс ±30%
        double baseDays = s.cost / 10000.0;
        double factor   = randDouble(0.7, 1.3);
        s.duration = max(1, (int)round(baseDays * factor));
        s.duration = min(s.duration, 365); // не более года

        // Предоплата 25–50% от стоимости, округлённая до 100 руб.
        double prepPct = randDouble(0.25, 0.50);
        s.prepayment = round(s.cost * prepPct / 100.0) * 100.0;

        return s;
    }

    vector<ITService> generateBatch(int n) {
        vector<ITService> batch;
        batch.reserve(n);
        for (int i = 0; i < n; ++i)
            batch.push_back(generate());
        return batch;
    }
};

// Запись в CSV
void writeCSV(const string& path, const vector<ITService>& data) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) { printf("Не удалось открыть: %s\n", path.c_str()); return; }

    // Заголовок
    fprintf(f, "name,cost,duration_days,prepayment\n");

    for (const auto& s : data) {
        // В названиях нет запятых, можно выводить как есть
        fprintf(f, "%s,%.2f,%d,%.2f\n", s.name.c_str(), s.cost, s.duration, s.prepayment);
    }
    fclose(f);

    printf("  Записано %zu строк -> %s\n", data.size(), path.c_str());
}

int main() {
    // 15 наборов: от 100 до 100 000 элементов
    const vector<int> SIZES = {
        100, 500, 1000, 5000, 10000,
        25000, 50000, 100000, 250000, 500000, 1000000
    };

    const string OUT_DIR = "datasets";
    filesystem::create_directories(OUT_DIR);

    ServiceGenerator gen(/*seed=*/2024);

    printf("Генерация наборов данных IT-услуг...\n");
    for (int sz : SIZES) {
        string path = OUT_DIR + "/services_" + to_string(sz) + ".csv";
        auto batch = gen.generateBatch(sz);
        writeCSV(path, batch);
    }

    printf("\nГотово! Создано %zu файлов в папке '%s'\n", SIZES.size(), OUT_DIR.c_str());
    return 0;
}
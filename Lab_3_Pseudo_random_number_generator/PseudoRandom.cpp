/**
 * @file RandomTestModified.cpp
 * @brief Сравнение трёх модифицированных генераторов псевдослучайных чисел
 *        с тестами NIST.
 * @details Генераторы:
 *          - MLCG (модифицированный LCG с обратным элементом),
 *          - Andxorshift (модифицированный XORShift с AND-операцией),
 *          - MXG521 (Mersenne Xorgens, комбинация Mersenne Twister и Xorgens).
 *          Для каждого генерируется 20 выборок по 1000 чисел в [0, 9999],
 *          вычисляются среднее, СКО, CV, выполняется тест хи-квадрат.
 *          Затем генерируется 1 000 000 бит и выполняются 5 тестов NIST.
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <cstdint>
#include <chrono>
#include <string>

using namespace std;

/// Число выборок для статистических тестов
const int NUM_SAMPLES = 20;
/// Размер одной выборки
const int SAMPLE_SIZE = 1000;
/// Диапазон генерируемых целых чисел [0, RANGE_SIZE-1]
const int RANGE_SIZE  = 10000;   
/// Количество интервалов для критерия хи-квадрат
const int K = 20;      
/// Ширина одного интервала
const int BIN_SIZE = RANGE_SIZE / K;   
/// Критическое значение chi^2 для df=19 и α=0.05
const double CHI2_CRIT = 30.1435;  


/**
 * @brief Модифицированный линейный конгруэнтный генератор (MLCG)
 * @details Использует стандартный шаг LCG с последующим обратным модульным
 *          преобразованием для нечётных состояний.
 */
class MLCG {
    uint32_t state;
    static const uint64_t a = 71;
    static const uint64_t c = 59;
    static const uint64_t MOD = 1ULL << 32;

    /**
     * @brief Вычисление обратного элемента по модулю 2^32
     * @param x нечётное число
     * @return x^{-1} mod 2^32
     */
    uint32_t modInverse32(uint32_t x) const {
        int64_t t = 0, newt = 1;
        uint64_t r = MOD, newr = x;
        while (newr != 0) {
            uint64_t quotient = r / newr;
            int64_t tmp = newt;
            newt = t - quotient * newt;
            t = tmp;
            uint64_t tmp_r = newr;
            newr = r - quotient * newr;
            r = tmp_r;
        }
        if (t < 0) t += MOD;
        return static_cast<uint32_t>(t);
    }

public:
    explicit MLCG(uint32_t seed = 12345) : state(seed) {}
    void seed(uint32_t s) { state = s; }
    /**
     * @brief Генерация следующего 32-битного числа
     * @return псевдослучайное число
     */
    uint32_t next() {
        state = static_cast<uint32_t>((a * state + c) % MOD);
        if (state & 1)
            return modInverse32(state);
        else
            return state;
    }
};


/**
 * @brief Модифицированный XORShift с AND-операцией (Andxorshift)
 * @details Рекуррента Xorshift дополнена битовым AND с константой,
 *          что расширяет пространство выбора и повышает вес Хэмминга
 *          эквивалентного LFSR.
 */
class Andxorshift {
    static const int N = 2;  ///< длина состояния (n)         
    static const int R = 1;  ///< задержка (0 < r < n)         
    static const uint32_t VEC = 0x7fffffe0; ///< константа для AND
    uint32_t s[N];
    int idx;

public:
    explicit Andxorshift(uint32_t seed = 12345) { seed_state(seed); }
    void seed(uint32_t seed) { seed_state(seed); }

    /**
     * @brief Генерация следующего 32-битного числа
     * @return псевдослучайное число
     */
    uint32_t next() {
        int i = idx;
        int j = (i + N - 1) % N;
        int k = (i + N - 1 + R) % N;

        uint32_t t = s[j];
        t ^= (t << 9) ^ (t >> 7);
        t ^= s[k] & VEC;

        s[i] = t;
        idx = (idx + 1) % N;
        return t;
    }

private:
    void seed_state(uint32_t seed) {
        MLCG init(seed);
        for (int i = 0; i < N; ++i) {
            s[i] = init.next() | 1;     ///< гарантируем нечётность
        }
        idx = 0;
    }
};

/**
 * @brief Генератор MXG521 — Mersenne Xorgens с размером состояния 521 бит
 * @details Объединяет рекурренту Xorgens с  Mersenne Twister
 *          и нелинейным темперированием Вейля.         
 */
class MXG521 {
    static const int N = 17; ///< число слов в состоянии             
    static const int R = 23; ///< недостающие биты (w-r = 9 старших)             
    static const int M = 10; ///< шаг             
    static const int A = 11, B = 15;   ///< левый/правый сдвиг для левой части   
    static const int C = 14, D = 11;   ///< левый/правый сдвиг для правой части   
    static const uint32_t UPPER_MASK = 0xFF800000;  ///< старшие 9 бит
    static const uint32_t LOWER_MASK = 0x007FFFFF;  ///< младшие 23 бита
    static const uint32_t WEYL_CONST = 0x9e3779b9;  ///< золотое сечение (32 бита)
    static const int WEYL_SHIFT = 16;   ///< сдвиг для Вейля   

    uint32_t state[N];
    uint32_t weyl_state;
    int idx;                             

public:
    explicit MXG521(uint32_t seed = 12345) { seed_state(seed); }
    void seed(uint32_t seed) { seed_state(seed); }

    /**
     * @brief Генерация следующего 32-битного числа
     * @return псевдослучайное число
     */
    uint32_t next() {
        int i = idx;
        int i1 = (i + 1) % N;
        int im = (i + M) % N;

        uint32_t y = (state[i] & UPPER_MASK) | (state[i1] & LOWER_MASK);
    
        y ^= (y << A);
        y ^= (y >> B);

        uint32_t t = state[im];
        t ^= (t << C);
        t ^= (t >> D);
        uint32_t new_word = t ^ y;
        state[i] = new_word;
        weyl_state ^= weyl_state >> WEYL_SHIFT;
        weyl_state += WEYL_CONST;
        uint32_t output = new_word + weyl_state; // 32-битное сложение (mod 2^32)

        idx = i1; // следующий индекс
        return output;
    }

private:
    void seed_state(uint32_t seed) {
        MLCG init(seed);
        for (int i = 0; i < N; ++i) {
            state[i] = init.next();
        }
        weyl_state = init.next() | 1;
        idx = 0;
        for (int i = 0; i < 1000; ++i) next();
    }
};

/**
 * @brief Статистики одной выборки
 */
struct SampleStats {
    double mean;  ///< среднее арифметическое
    double stddev;  ///< стандартное отклонение
    double cv;  ///< коэффициент вариации
    double chi2; ///< статистика хи-квадрат
    bool   uniform; ///< пройден ли тест на равномерность
};

/**
 * @brief Вычисление описательных статистик и теста хи-квадрат
 * @param sample выборка целых чисел
 * @return структура SampleStats
 */
SampleStats computeStats(const vector<int>& sample) {
    int n = sample.size();
    double sum = 0.0;
    for (int x : sample) sum += x;
    double mean = sum / n;

    double sq = 0.0;
    for (int x : sample) sq += (x - mean) * (x - mean);
    double stddev = sqrt(sq / (n - 1));

    double cv = (mean != 0.0) ? (stddev / mean) : 0.0;

    int bins[K] = {0};
    for (int x : sample) {
        int bin = x / BIN_SIZE;
        if (bin >= K) bin = K - 1;
        ++bins[bin];
    }

    double expected = double(n) / K;
    double chi2 = 0.0;
    for (int i = 0; i < K; ++i) {
        double diff = bins[i] - expected;
        chi2 += (diff * diff) / expected;
    }

    bool uniform = (chi2 <= CHI2_CRIT);
    return {mean, stddev, cv, chi2, uniform};
}

/**
 * @brief Генерация выборки целых чисел в заданном диапазоне
 * @param gen генератор (должен иметь метод next() -> uint32_t)
 * @param size размер выборки
 * @param range верхняя граница диапазона (не включается)
 * @return вектор сгенерированных чисел
 */
template<typename Gen>
vector<int> generateSample(Gen& gen, int size, int range) {
    vector<int> sample;
    sample.reserve(size);
    for (int i = 0; i < size; ++i)
        sample.push_back(gen.next() % range);
    return sample;
}

/**
 * @brief Дополнительная функция ошибок (erfc) с высокой точностью
 * @param x аргумент
 * @return значение erfc(x)
 */
double erfc(double x) {
    double t = 1.0 / (1.0 + 0.5 * fabs(x));
    double tau = t * exp(-x*x - 1.26551223 + t*(1.00002368 + t*(0.37409196 +
                 t*(0.09678418 + t*(-0.18628806 + t*(0.27886807 + t*(-1.13520398 +
                 t*(1.48851587 + t*(-0.82215223 + t*0.17087277)))))))));
    return (x >= 0) ? tau : 2.0 - tau;
}

/**
 * @brief Вычисление p‑value из нормальной статистики z
 * @param z значение статистики
 * @param двусторонний тест (по умолчанию true)
 * @return p‑value
 */
double p_value_from_normal(double z, bool two_sided = true) {
    double p = erfc(fabs(z) / sqrt(2.0));
    return two_sided ? p : p / 2.0;
}


/**
 * @brief Вычисление p‑value из статистики хи-квадрат
 * @param chi2 значение хи-квадрат
 * @param df число степеней свободы
 * @return приближённое p‑value
 */
double p_value_from_chi2(double chi2, int df) {
    double z = (sqrt(2.0 * chi2) - sqrt(2.0 * df - 1.0));
    return erfc(fabs(z) / sqrt(2.0));
}

/**
 * @brief Частотный побитовый тест (Monobit)
 * @param bits вектор битов (0/1)
 * @return p‑value
 */
double nist_monobit(const vector<uint8_t>& bits) {
    int n = bits.size();
    if (n < 100) return -1;
    int sum = 0;
    for (auto b : bits) sum += b ? 1 : -1;
    double s = sum / sqrt(n);
    double p = erfc(fabs(s) / sqrt(2.0));
    return p;
}

/**
 * @brief Частотный блочный тест (Block Frequency)
 * @param bits вектор битов
 * @param M размер блока (по умолчанию 128)
 * @return p‑value
 */
double nist_block_frequency(const vector<uint8_t>& bits, int M = 128) {
    int n = bits.size();
    if (n < M) return -1;
    int N = n / M;
    double chi2 = 0.0;
    for (int i = 0; i < N; ++i) {
        int ones = 0;
        for (int j = 0; j < M; ++j)
            if (bits[i*M + j]) ++ones;
        double pi = (double)ones / M;
        chi2 += (pi - 0.5) * (pi - 0.5);
    }
    chi2 *= 4.0 * M;
    return p_value_from_chi2(chi2, N);
}

/**
 * @brief Тест на последовательности одинаковых битов (Runs)
 * @param bits вектор битов
 * @return p‑value
 */
double nist_runs(const vector<uint8_t>& bits) {
    int n = bits.size();
    if (n < 100) return -1;
    int ones = 0;
    for (auto b : bits) ones += b;
    double pi = (double)ones / n;
    if (fabs(pi - 0.5) >= 2.0 / sqrt(n)) return 0.0;
    int runs = 1;
    for (int i = 1; i < n; ++i)
        if (bits[i] != bits[i-1]) ++runs;
    double num = runs - 2.0 * n * pi * (1.0 - pi);
    double den = 2.0 * sqrt(2.0 * n) * pi * (1.0 - pi);
    double z = num / den;
    return erfc(fabs(z) / sqrt(2.0));
}

/**
 * @brief Тест на самую длинную последовательность единиц в блоке (Longest Run)
 * @param bits вектор битов
 * @return p‑value
 */
double nist_longest_run(const vector<uint8_t>& bits) {
    int n = bits.size();
    if (n < 128) return -1;

    const int M = 10000;          
    int N = n / M;                
    const int K = 7;              
    vector<int> v(K, 0);          

    for (int i = 0; i < N; ++i) {
        int longest = 0, current = 0;
        for (int j = 0; j < M; ++j) {
            if (bits[i*M + j]) {
                ++current;
            } else {
                if (current > longest) longest = current;
                current = 0;
            }
        }
        if (current > longest) longest = current;
        if (longest <= 10)       v[0]++;
        else if (longest == 11)  v[1]++;
        else if (longest == 12)  v[2]++;
        else if (longest == 13)  v[3]++;
        else if (longest == 14)  v[4]++;
        else if (longest == 15)  v[5]++;
        else                     v[6]++;   // >= 16
    }

  
    double pi[] = {0.0882, 0.2092, 0.2483, 0.1933, 0.1208, 0.0675, 0.0727};
    double chi2 = 0.0;
    for (int i = 0; i < K; ++i) {
        double expected = N * pi[i];
        chi2 += (v[i] - expected) * (v[i] - expected) / expected;
    }
    
    return p_value_from_chi2(chi2, K - 1);
}

/**
 * @brief Тест кумулятивных сумм (CUSUM)
 * @param bits вектор битов
 * @param forward направление (true – прямое, false – обратное)
 * @return p‑value
 */
double nist_cusum(const vector<uint8_t>& bits, bool forward = true) {
    int n = bits.size();
    if (n < 100) return -1;
    vector<int> X(n, 0);
    int sum = 0;
    if (forward) {
        for (int i = 0; i < n; ++i) {
            sum += bits[i] ? 1 : -1;
            X[i] = sum;
        }
    } else {
        for (int i = n-1; i >= 0; --i) {
            sum += bits[i] ? 1 : -1;
            X[n-1-i] = sum;
        }
    }
    int maxZ = 0;
    for (auto z : X) if (abs(z) > maxZ) maxZ = abs(z);
    double z_stat = maxZ / sqrt(n);
    return erfc(z_stat / sqrt(2.0));
}

/**
 * @brief Обёртка над стандартным rand() для получения 32-битных чисел
 */
class StandardRand {
public:
    explicit StandardRand(unsigned seed = 12345) { srand(seed); }
    void seed(unsigned seed) { srand(seed); }
    uint32_t next() {
        uint32_t hi = (static_cast<uint32_t>(rand()) & 0xFFFF) << 16;
        uint32_t lo = static_cast<uint32_t>(rand()) & 0xFFFF;
        return hi | lo;
    }
};

/**
 * @brief Замер времени генерации N чисел
 * @param gen генератор
 * @param count количество чисел
 * @return время в миллисекундах
 */
template<typename Gen>
double measureGenerationTime(Gen& gen, int count) {
    auto t0 = chrono::high_resolution_clock::now();
    for (int i = 0; i < count; ++i) {
        volatile uint32_t sink = gen.next();  // предотвращаем оптимизацию
    }
    auto t1 = chrono::high_resolution_clock::now();
    return chrono::duration<double, milli>(t1 - t0).count();
}

/**
 * @brief Генерация битового вектора из генератора (старший бит)
 * @param gen генератор
 * @param numBits количество бит
 * @return вектор битов
 */
template<typename Gen>
vector<uint8_t> generateBits(Gen& gen, int numBits) {
    vector<uint8_t> bits;
    bits.reserve(numBits);
    for (int i = 0; i < numBits; ++i) {
        uint32_t r = gen.next();
        bits.push_back((r >> 31) & 1);  
    }
    return bits;
}

/**
 * @brief Точка входа в программу
 * @details Выполняет все три части эксперимента:
 *          1. Описательная статистика и χ².
 *          2. Тесты NIST.
 *          3. Замеры скорости.
 * @return 0 при успешном завершении
 */
int main() {
    auto now = chrono::high_resolution_clock::now().time_since_epoch().count();
    uint32_t baseSeed = static_cast<uint32_t>(now);

    FILE* f = fopen("random_stats.csv", "w");
    fprintf(f, "generator,sample,mean,stddev,cv,chi2,uniform\n");

    printf("MLCG (Modified LCG) results:\n");
    for (int s = 0; s < NUM_SAMPLES; ++s) {
        MLCG gen(baseSeed + s * 1000);
        auto sample = generateSample(gen, SAMPLE_SIZE, RANGE_SIZE);
        auto stats = computeStats(sample);
        printf("Sample %2d: mean=%7.1f  stddev=%7.1f  cv=%6.4f  chi2=%7.3f  %s\n",
               s+1, stats.mean, stats.stddev, stats.cv, stats.chi2,
               stats.uniform ? "PASS" : "FAIL");
        fprintf(f, "MLCG,%d,%.1f,%.1f,%.4f,%.3f,%d\n",
                s+1, stats.mean, stats.stddev, stats.cv, stats.chi2, stats.uniform);
    }

    printf("\nAndxorshift (Modified XORShift) results:\n");
    for (int s = 0; s < NUM_SAMPLES; ++s) {
        Andxorshift gen(baseSeed + s * 2000);
        auto sample = generateSample(gen, SAMPLE_SIZE, RANGE_SIZE);
        auto stats = computeStats(sample);
        printf("Sample %2d: mean=%7.1f  stddev=%7.1f  cv=%6.4f  chi2=%7.3f  %s\n",
               s+1, stats.mean, stats.stddev, stats.cv, stats.chi2,
               stats.uniform ? "PASS" : "FAIL");
        fprintf(f, "Andxorshift,%d,%.1f,%.1f,%.4f,%.3f,%d\n",
                s+1, stats.mean, stats.stddev, stats.cv, stats.chi2, stats.uniform);
    }

    printf("\nMXG521 (Mersenne Xorgens, 521 bit) results:\n");
    for (int s = 0; s < NUM_SAMPLES; ++s) {
        MXG521 gen(baseSeed + s * 3000);
        auto sample = generateSample(gen, SAMPLE_SIZE, RANGE_SIZE);
        auto stats = computeStats(sample);
        printf("Sample %2d: mean=%7.1f  stddev=%7.1f  cv=%6.4f  chi2=%7.3f  %s\n",
               s+1, stats.mean, stats.stddev, stats.cv, stats.chi2,
               stats.uniform ? "PASS" : "FAIL");
        fprintf(f, "MXG521,%d,%.1f,%.1f,%.4f,%.3f,%d\n",
                s+1, stats.mean, stats.stddev, stats.cv, stats.chi2, stats.uniform);
    }
    fclose(f);
    printf("\nDescriptive statistics saved to random_stats.csv\n");

    
    const int BIT_COUNT = 1000000;
    vector<string> generators = {"MLCG", "Andxorshift", "MXG521"};
    FILE* nist_f = fopen("nist_results.csv", "w");
    fprintf(nist_f, "generator,monobit,block_freq,runs,longest_run,cusum_forward,cusum_backward\n");

    for (int g = 0; g < 3; ++g) {
        vector<uint8_t> bits;
        uint32_t seed = baseSeed + g * 5000;
        if (g == 0) {
            MLCG gen(seed);
            bits = generateBits(gen, BIT_COUNT);
        } else if (g == 1) {
            Andxorshift gen(seed);
            bits = generateBits(gen, BIT_COUNT);
        } else {
            MXG521 gen(seed);
            bits = generateBits(gen, BIT_COUNT);
        }

        double mono = nist_monobit(bits);
        double block = nist_block_frequency(bits, 128);
        double runs = nist_runs(bits);
        double longest = nist_longest_run(bits);
        double cusum_f = nist_cusum(bits, true);
        double cusum_b = nist_cusum(bits, false);

        printf("\n%s NIST results (p-values):\n", generators[g].c_str());
        printf("  Monobit:           %.6f\n", mono);
        printf("  Block Frequency:   %.6f\n", block);
        printf("  Runs:              %.6f\n", runs);
        printf("  Longest Run:       %.6f\n", longest);
        printf("  CUSUM forward:     %.6f\n", cusum_f);
        printf("  CUSUM backward:    %.6f\n", cusum_b);

        fprintf(nist_f, "%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                generators[g].c_str(), mono, block, runs, longest, cusum_f, cusum_b);
    }
    fclose(nist_f);
    printf("\nNIST results saved to nist_results.csv\n");
   const vector<int> SIZES = {
        1000, 2000, 5000, 10000, 20000, 50000, 100000,
        200000, 500000, 1000000
    };

    FILE* speed_f = fopen("speed_results.csv", "w");
    fprintf(speed_f, "size,MLCG_ms,Andxorshift_ms,MXG521_ms,StandardRand_ms\n");
    printf("\n%s%12s%12s%12s%12s\n", "Size", "MLCG", "Andxorshift", "MXG521", "StdRand");

    for (int sz : SIZES) {
        double tMLCG, tAnd, tMXG, tStd;

        
        {
            MLCG gen(baseSeed);
            tMLCG = measureGenerationTime(gen, sz);
        }
        
        {
            Andxorshift gen(baseSeed + 1);
            tAnd = measureGenerationTime(gen, sz);
        }
        {
            MXG521 gen(baseSeed + 2);
            tMXG = measureGenerationTime(gen, sz);
        }
        
        {
            StandardRand gen(baseSeed + 3);
            tStd = measureGenerationTime(gen, sz);
        }

        printf("%10d%12.3f%12.3f%12.3f%12.3f\n", sz, tMLCG, tAnd, tMXG, tStd);
        fprintf(speed_f, "%d,%.3f,%.3f,%.3f,%.3f\n", sz, tMLCG, tAnd, tMXG, tStd);
    }

    fclose(speed_f);
    printf("\nSpeed results saved to speed_results.csv\n");

    return 0;
}
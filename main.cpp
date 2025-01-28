#include <concepts> // std::floating_point
#include <cassert> // assert, static_assert
#include <cstdint> // int32_t
#include <iostream> // std::cout
#include <iomanip> // std::fixed, std::setprecision
#include <limits> // std::numeric_limits
#include <string> // std::string 
#include <string_view> // std::string_view
#include <cmath> // std::log10, std::pow
#include <ranges> // std::views::iota
#include <algorithm> // std::clamp
#include <numeric> // std::numeric_limits

static_assert( std::numeric_limits<float>::is_iec559 );
static_assert( std::numeric_limits<double>::is_iec559 );

// https://habr.com/ru/articles/131977/
class MakeString {
public:
    template<class T>
    MakeString& operator<<(const T& arg) {
        m_stream << arg;
        return *this;
    }
    operator std::string() const {
        return m_stream.str();
    }
private:
    std::stringstream m_stream;
};

// https://stackoverflow.com/questions/39821367/very-fast-approximate-logarithm-natural-log-function-in-c
static double log10_fast_ankerl(double a) // Практичный логарифм по основанию 10.
{
  typedef int32_t my_int;
  static_assert(sizeof(double) == 2*sizeof(my_int));
  union { double d; my_int x[2]; } u = { a };
  if constexpr (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
    return (u.x[1] - 1072632447.) * 6.610368362777016e-7 / std::log(10.);
  } else if constexpr (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
    return (u.x[0] - 1072632447.) * 6.610368362777016e-7 / std::log(10.);
  } else {
    assert((1 == 0) && "Endianess must be either Little or Big.");
  }
}

static auto CalcNegativePrecision(std::floating_point auto x) {
  x = std::abs(x);
  const auto offset = (decltype(x))(0.5);
  const auto threshold = (decltype(x))(10.);
  const int init_precision = static_cast<int>( offset +  (x < threshold ? log10_fast_ankerl(x + 1) : log10_fast_ankerl(x)) );
  return init_precision + (init_precision < 1 ? 1 : 0);
}

static auto MyRound(std::floating_point auto x) {
  return std::floor( x + (decltype(x))(0.5) );
};

static int EstimatePrecision(std::floating_point auto x) {
  x = std::abs(x);
  int precision = 0; // Количество знаков после запятой.
  const int negative_precision = CalcNegativePrecision(x); // ~Количество знаков перед запятой.
  constexpr auto epsilon = std::numeric_limits<decltype(x)>::epsilon();
  constexpr int max_digits = std::numeric_limits<decltype(x)>::max_digits10;
  auto rounded_x = MyRound(x);
  auto initial_pow10 = std::pow(10, negative_precision);
  while (precision < max_digits - negative_precision) {
      if (std::abs(x - rounded_x) < epsilon * initial_pow10) break;
      precision++;
      initial_pow10 *= 10;
      x -= rounded_x; x *= 10; rounded_x = MyRound(x);
  }
  return std::clamp(precision, 0, max_digits);
}

/**
 * Преобразовать число с плавающей запятой в строку.
 */
static std::string FloatToString(std::floating_point auto x) {
  const int precision = EstimatePrecision(x);
  return (std::string)(MakeString() << std::fixed << std::setprecision(precision) << x);
}

/**
 * Класс для "чистой" печати чисел с плавающей запятой.
 * Скрывает мусорные знаки, которые по ошибке меньше соответствующего "епсилон".
 */
class FloatView {
public:
  /**
   * Конструктор.
   */
  explicit FloatView(std::floating_point auto x)
  : mStrValue {FloatToString(x)} {}
  
  /**
   * Установить новое значение.
   */
  void SetValue(std::floating_point auto x) {
    mStrValue = FloatToString(x);
  }

  /**
   * Получить вид на строковое представление числа.
   */
  std::string_view View() const noexcept {
    return mStrValue;
  }
private:
  /**
   * Строковое представление числа.
   */
  std::string mStrValue;
};


int main() {
  using namespace std::literals;

  assert(CalcNegativePrecision(0.) == 1);
  assert(CalcNegativePrecision(1.) == 1);
  assert(CalcNegativePrecision(10.) == 1);
  assert(CalcNegativePrecision(33.) == 2);
  assert(CalcNegativePrecision(99.) == 2);
  assert(CalcNegativePrecision(100.) == 2);

  FloatView f{0.2};
  std::cout << "Double: " << f.View() << '\n';
  assert(f.View() == "0.2"sv);

  f.SetValue(0.2f);
  std::cout << "Float: " << f.View() << '\n';
  assert(f.View() == "0.2"sv);

  f.SetValue(0.1);
  std::cout << "Double: " << f.View() << '\n';
  assert(f.View() == "0.1"sv);

  f.SetValue(0.1f);
  std::cout << "Float: " << f.View() << '\n';
  assert(f.View() == "0.1"sv);

  {
    const double s = 0.1 + 0.2;
    f.SetValue(s);
    std::cout << "0.1 + 0.2: " << f.View() << '\n';
    assert(f.View() == "0.3"sv);
  }

  {
    double s = 0;
    for (auto _ : std::views::iota(0, 5)) {
      s += 0.1;
    }
    f.SetValue(s);
    std::cout << "Half: " << f.View() << '\n';
    assert(f.View() == "0.5"sv);
  }

  {
    double s = 0;
    for (auto _ : std::views::iota(0, 10)) {
      s += 0.1;
    }
    f.SetValue(s);
    std::cout << "Unit: " << f.View() << '\n';
    assert(f.View() == "1"sv);
  }

  {
    double s = 0;
    for (auto _ : std::views::iota(0, 99)) {
      s += 0.01;
    }
    f.SetValue(s);
    std::cout << "Sum of 99 by 0.01: " << f.View() << '\n';
    assert(f.View() == "0.99"sv);
  }

  {
    double s = 0;
    for (auto _ : std::views::iota(0, 999)) {
      s += 0.001;
    }
    f.SetValue(s);
    std::cout << "Sum of 999 by 0.001: " << f.View() << '\n';
    assert(f.View() == "0.999"sv);
  }

  {
    double s = 0;
    for (auto _ : std::views::iota(0, 9999)) {
      s += 0.0001;
    }
    f.SetValue(s);
    FloatView g{0.9999};
    std::cout << "Min viewable summation error: double: " << f.View() << " vs exact. " << g.View()  << '\n';
  }

  {
    float s = 0;
    for (auto _ : std::views::iota(0, 999)) {
      s += 0.001f;
    }
    f.SetValue(s);
    FloatView g{0.999f};
    std::cout << "Min viewable summation error: float: " << f.View() << " vs exact. " << g.View()  << '\n';
  }

  {
    f.SetValue(0.99999999999999);
    std::cout << "Most close to 1: " << f.View() << '\n';
    assert(f.View() == "0.99999999999999"sv);
  }

  {
    f.SetValue(-0.99999999999999);
    std::cout << "Most close to -1: " << f.View() << '\n';
    assert(f.View() == "-0.99999999999999"sv);
  }

  {
    f.SetValue(999999999999999.);
    std::cout << "Most 9's: " << f.View() << '\n';
    assert(f.View() == "999999999999999"sv);
  }

  {
    f.SetValue(-999999999999999.);
    std::cout << "Negative most 9's: " << f.View() << '\n';
    assert(f.View() == "-999999999999999"sv);
  }

  {
    f.SetValue(0.00000000000001);
    std::cout << "Minimal non-zero: " << f.View() << '\n';
    assert(f.View() == "0.00000000000001"sv);
  }

  {
    f.SetValue(-0.00000000000001);
    std::cout << "Negative max closer to zero: " << f.View() << '\n';
    assert(f.View() == "-0.00000000000001"sv);
  }

  {
    f.SetValue(0.303);
    std::cout << "Some double: " << f.View() << '\n';
    assert(f.View() == "0.303"sv);
  }

  {
    f.SetValue(0.505050505);
    std::cout << "Some double: " << f.View() << '\n';
    assert(f.View() == "0.505050505"sv);
  }

  {
    f.SetValue(0.);
    std::cout << "Zero: " << f.View() << '\n';
    assert(f.View() == "0"sv);
  }

  {
    f.SetValue(-0.);
    std::cout << "Negative zero: " << f.View() << '\n';
    assert(f.View() == "-0"sv);
  }

  {
    f.SetValue(1. / 0.);
    std::cout << "Infinity: " << f.View() << '\n';
    assert(f.View() == "inf"sv);
  }

  {
    f.SetValue(std::sqrt(-1.));
    std::cout << "NaN: " << f.View() << '\n';
    assert(f.View() == "-nan"sv || f.View() == "nan"sv);
  }

  {
    f.SetValue(std::log(-1.));
    std::cout << "NaN: " << f.View() << '\n';
    assert(f.View() == "-nan"sv || f.View() == "nan"sv);
  }

  std::cout << "Ok!\n";

  return EXIT_SUCCESS;
}

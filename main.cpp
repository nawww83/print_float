#include <concepts> // std::floating_point
#include <cassert> // assert, static_assert
#include <cstdint> // int32_t
#include <iostream> // std::cout
#include <limits> // std::numeric_limits
#include <string> // std::string 
#include <string_view> // std::string_view
#include <cmath> // std::floor
#include <algorithm> // std::clamp
#include <bit> // std::bit_ceil, std::bit_width
#include <stdio.h> // sprintf

static_assert( std::numeric_limits<float>::is_iec559 );
static_assert( std::numeric_limits<double>::is_iec559 );

/**
 * Приближенный логарифм по основанию 10.
 */
static double log10_abs(double value) {
    if (!std::isfinite(value)) {
      return value;
    }
    value = std::abs(value);
    const int deg2 = std::ilogb(value + 1);
    return std::log(2) * deg2 / std::log(10);
}

static auto CalcNegativePrecision(std::floating_point auto x) {
  const auto offset = (decltype(x))(0.5);
  return static_cast<int>( offset + log10_abs(x) );
}

static auto MyRound(std::floating_point auto x) {
  return std::floor( x + (decltype(x))(0.5) );
};

static int EstimatePrecision(std::floating_point auto x) {
  int precision = 0; // Количество знаков после запятой.
  const int negative_precision = CalcNegativePrecision(x); // ~Количество знаков перед запятой.
  constexpr auto epsilon = std::numeric_limits<decltype(x)>::epsilon();
  constexpr int max_digits = std::numeric_limits<decltype(x)>::digits10;
  x = std::abs(x);
  auto rounded_x = MyRound(x);
  decltype(x) max_expected_error = (x < 1 ? 1 : x) * epsilon;
  for (; precision <= (max_digits - negative_precision); precision++) {
      if (std::abs(x - rounded_x) < max_expected_error) break;
      max_expected_error *= 10;
      x -= rounded_x; x *= 10;
      rounded_x = MyRound(x);
  }
  return std::clamp(precision, 0, max_digits);
}

/**
 * Класс для "чистой" печати чисел с плавающей запятой.
 * Скрывает мусорные знаки, которые по ошибке меньше соответствующего "епсилон".
 */
class FloatView {
public:
  /**
   * Конструктор по умолчанию.
   */
  explicit FloatView() = default;

  /**
   * Конструктор с параметром.
   */
  explicit FloatView(std::floating_point auto x) {
    FloatToString(x);
  }
  
  /**
   * Установить новое значение.
   */
  void SetValue(std::floating_point auto x) {
    FloatToString(x);
  }

  /**
   * Получить вид на строковое представление числа.
   */
  std::string_view View() const {
    return mStrValue;
  }

  /**
   * Получить точность текущего числа.
   */
  int Precision() const {
    return mPrecision;
  }

private:
  /**
  * Преобразовать число с плавающей запятой в строку.
  */
  void FloatToString(std::floating_point auto x) {
    mPrecision = EstimatePrecision(x);
    FillStringByNumber(x);
  }

  /**
   * Вспомогательная функция заполнения строки числом с плавающей точкой с заданной точностью.
   */
  void FillStringByNumber(std::floating_point auto x) {
    assert(mPrecision >= 0);
    assert(mPrecision <= std::numeric_limits<decltype(x)>::max_digits10);
    // 1u - нуль-терминатор, 1u - знак "0" перед точкой, 2u - два возможных знака: "-" и "."
    char buffer[ std::bit_ceil( 1u + 1u + 2u + std::numeric_limits<decltype(x)>::max_exponent10 ) ];
    std::string fmt {"%."};
    fmt.append( std::to_string( mPrecision ) );
    if constexpr ( std::is_same_v<float, decltype(x)>) {
      fmt.append("f");
    }
    else if constexpr (std::is_same_v<double, decltype(x)>) {
      fmt.append("lf");
    }
    else if constexpr (std::is_same_v<long double, decltype(x)>) {
      fmt.append("Lf");
    }
    const int printed_size = sprintf(buffer, fmt.c_str(), x);
    mStrValue.assign(buffer, printed_size);
  }

  /**
   * Строковое представление числа.
   */
  std::string mStrValue;

  /**
   * Точность числа - количество знаков после запятой.
   */
  int mPrecision = 0;
};


int main() {
  using namespace std::literals;

  FloatView f{0.3 * 9 * 10 * 11 + 3.};
  std::cout << "Double 300: " << f.View() << '\n';
  assert(f.View() == "300"sv);

  {
    double x = 0.;
      for (int i=0; i<22; ++i) {
          x += 0.3;
          x *= 2.;
      }
      f.SetValue(x);
      std::cout << "Double sum 2516581.8: " << f.View() << ", precision: " << f.Precision() << '\n';
      assert(f.View() == "2516581.8"sv);
  }

  {
    f.SetValue(0.50505f);
    std::cout << "Some float: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.50505"sv);
  }

  {
    f.SetValue(0.505050505050505);
    std::cout << "Some double: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.505050505050505"sv);
  }

  {
    f.SetValue(0.50505050505050505L);
    std::cout << "Some long double: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.50505050505050505"sv);
  }

  {
    f.SetValue(0.0505050f);
    std::cout << "Some float: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.050505"sv);
  }

  {
    f.SetValue(0.050505050505050);
    std::cout << "Some double: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.05050505050505"sv);
  }

  {
    f.SetValue(0.0505050505050505050L);
    std::cout << "Some long double: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.050505050505050505"sv);
  }

  {
    f.SetValue(0.123456f);
    std::cout << "Some float: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.123456"sv);
  }

  {
    f.SetValue(0.123456789012345);
    std::cout << "Some double: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.123456789012345"sv);
  }

  {
    f.SetValue(0.123456789012345678L);
    std::cout << "Some long double: " << f.View() << ", precision: " << f.Precision() << '\n';
    assert(f.View() == "0.123456789012345678"sv);
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

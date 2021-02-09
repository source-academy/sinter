function is_nan(x) {
  return is_number(x) && !(x < 0) && !(x >= 0);
}

function check_rand(x) {
  return is_number(x) && x >= 0 && x < 1;
}

function approx_equal(a, b) {
  return a == b || math_abs(a - b) < 0.0001;
}

display(approx_equal(math_abs(2.25), 2.25));
display(approx_equal(math_abs(-2.75), 2.75));
display(approx_equal(math_acos(0.25), 1.318116071652818));
display(approx_equal(math_acosh(2.25), 1.45057451382258));
display(approx_equal(math_asin(0.5), 0.5235987755982989));
display(approx_equal(math_asinh(2.25), 1.5501579568690622));
display(approx_equal(math_atan(2.25), 1.1525719972156676));
display(approx_equal(math_atan2(2.25, 5), 0.4228539261329407));
display(approx_equal(math_atanh(0.5), 0.5493061443340548));
display(approx_equal(math_cbrt(2.25), 1.3103706971044482));
display(approx_equal(math_ceil(2.25), 3));
display(approx_equal(math_ceil(2.75), 3));
display(approx_equal(math_clz32(0x0F000000), 4));
display(approx_equal(math_clz32(0), 32));
display(approx_equal(math_cos(2.25), -0.6281736227227391));
display(approx_equal(math_cos(2.75), -0.9243023786324636));
display(approx_equal(math_cosh(2.25), 4.796567530460195));
display(approx_equal(math_cosh(2.5), 6.132289479663686));
display(approx_equal(math_exp(2.25), 9.487735836358526));
display(approx_equal(math_exp(2.75), 15.642631884188171));
display(approx_equal(math_expm1(2.25), 8.487735836358526));
display(approx_equal(math_expm1(2.75), 14.642631884188171));
display(approx_equal(math_floor(2.25), 2));
display(approx_equal(math_floor(2.75), 2));
display(approx_equal(math_fround(2.25), 2.25));
display(approx_equal(math_fround(2.75), 2.75));
display(approx_equal(math_hypot(), 0));
display(approx_equal(math_hypot(2.25), 2.25));
display(approx_equal(math_hypot(2.25, 2.75), 3.553167600887974));
display(is_nan(math_hypot(2.25, 2.75, NaN)));
display(approx_equal(math_imul(32, 32), 1024));
display(approx_equal(math_imul(123, 123), 15129));
display(approx_equal(math_log(2.25), 0.8109302162163288));
display(approx_equal(math_log(2.75), 1.0116009116784799));
display(approx_equal(math_log1p(2.25), 1.1786549963416462));
display(approx_equal(math_log1p(2.75), 1.3217558399823195));
display(approx_equal(math_log2(2.25), 1.1699250014423124));
display(approx_equal(math_log2(2.75), 1.4594316186372973));
display(approx_equal(math_log10(2.25), 0.3521825181113625));
display(approx_equal(math_log10(2.75), 0.43933269383026263));
display(approx_equal(math_max(), -Infinity));
display(approx_equal(math_max(2.25), 2.25));
display(approx_equal(math_max(2.25, 5), 5));
display(approx_equal(math_max(2.25, 5, 5000), 5000));
display(approx_equal(math_max(2.25, 5, 5000, Infinity), Infinity));
display(approx_equal(math_min(), Infinity));
display(approx_equal(math_min(2.25), 2.25));
display(approx_equal(math_min(2.25, 5), 2.25));
display(approx_equal(math_min(2.25, 5, 5000), 2.25));
display(approx_equal(math_min(2.25, 5, 5000, Infinity), 2.25));
display(approx_equal(math_pow(2.25, 2.5), 7.59375));
display(approx_equal(math_pow(2.75, 2.5), 12.540987488531355));
display(check_rand(math_random()));
display(approx_equal(math_round(2.25), 2));
display(approx_equal(math_round(2.75), 3));
display(approx_equal(math_sign(2.25), 1));
display(approx_equal(math_sign(-2.75), -1));
display(approx_equal(math_sign(0), 0));
display(approx_equal(math_sin(2.25), 0.7780731968879212));
display(approx_equal(math_sinh(2.25), 4.691168305898331));
display(approx_equal(math_sqrt(2.75), 1.6583123951777));
display(approx_equal(math_tan(2.25), -1.2386276162240966));
display(approx_equal(math_tanh(2.25), 0.9780261147388136));
display(approx_equal(math_trunc(2.25), 2));
display(approx_equal(math_trunc(2.75), 2));

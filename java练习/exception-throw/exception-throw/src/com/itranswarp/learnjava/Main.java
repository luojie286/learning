package com.itranswarp.learnjava;

public class Main {

    public static void main(String[] args) {
        int[] salaries = { 2000, -200, 2000 };
        double[] rates = { 0.1, 0.1, -0.1 };

        // try 写在循环「里面」：一条出错不影响下一条
        for (int i = 0; i < salaries.length; i++) {
            try {
                System.out.println("第" + (i + 1) + "个: " + tax(salaries[i], rates[i]));
            } catch (IllegalArgumentException e) {
                System.out.println("第" + (i + 1) + "个: 出错 -> " + e.getMessage());
            }
        }

        System.out.println("【三条全部处理完毕，程序正常结束】");
    }

    static double tax(int salary, double rate) {
        if (salary < 0) {
            throw new IllegalArgumentException("salary必须为非负数");
        }
        if (rate < 0) {
            throw new IllegalArgumentException("rate必须为非负数");
        }
        return salary * rate;
    }
}

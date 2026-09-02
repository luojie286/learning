#include <iostream>
using namespace std;

// 命题逻辑运算函数
bool conjunction(bool P, bool Q) {      // 合取 P ∧ Q
    return P && Q;
}

bool disjunction(bool P, bool Q) {      // 析取 P ∨ Q
    return P || Q;
}

bool implication(bool P, bool Q) {       // 条件 P → Q （等价于 ¬P ∨ Q）
    return !P || Q;
}

bool biconditional(bool P, bool Q) {     // 双条件 P ↔ Q （等价于 (P→Q) ∧ (Q→P)）
    return (P == Q);
}

int main() {
    cout << "请输入命题 P 和 Q 的真值（0 表示假，1 表示真）：" << endl;
    int p_input, q_input;
    cin >> p_input >> q_input;

    // 输入合法性检查（可选）
    if (p_input != 0 && p_input != 1) {
        cout << "P 的输入无效！请使用 0 或 1。" << endl;
        return 1;
    }
    if (q_input != 0 && q_input != 1) {
        cout << "Q 的输入无效！请使用 0 或 1。" << endl;
        return 1;
    }

    bool P = (p_input == 1);
    bool Q = (q_input == 1);

    // 输出结果
    cout << "\n命题逻辑运算结果：" << endl;
    cout << "P ∧ Q(合取): " << conjunction(P, Q) << endl;
    cout << "P ∨ Q（析取）: " << disjunction(P, Q) << endl;
    cout << "P → Q（条件）: " << implication(P, Q) << endl;
    cout << "P ↔ Q（双条件）: " << biconditional(P, Q) << endl;

    return 0;
}

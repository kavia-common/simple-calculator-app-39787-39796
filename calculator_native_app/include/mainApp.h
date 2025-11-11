#ifndef MAIN_APP_H
#define MAIN_APP_H

#include <QWidget>
#include <QString>
#include <QVector>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QSizePolicy>

/*
 Ocean Professional Theme
 - primary: #2563EB (blue)
 - secondary: #F59E0B (amber)
 - error: #EF4444 (red)
 - background: #f9fafb (light gray)
 - surface: #ffffff (white)
 - text: #111827 (near-black)
*/

/**
 * PUBLIC_INTERFACE
 * CalculatorEngine provides core arithmetic parsing and stateful evaluation for basic calculator functionality.
 * It supports left-to-right chaining (no operator precedence), decimal input building, clear, backspace,
 * and safe division handling (division by zero returns error flag).
 */
class CalculatorEngine
{
public:
    CalculatorEngine();

    // PUBLIC_INTERFACE
    /**
     * Resets the calculator state completely.
     */
    void clearAll();

    // PUBLIC_INTERFACE
    /**
     * Deletes one character from the current input.
     * If current input becomes empty, it becomes "0".
     */
    void backspace();

    // PUBLIC_INTERFACE
    /**
     * Appends a digit (0-9) to the current input.
     */
    void inputDigit(int d);

    // PUBLIC_INTERFACE
    /**
     * Appends a decimal point if not already present.
     */
    void inputDecimal();

    // PUBLIC_INTERFACE
    /**
     * Queues an operation (+, -, *, /) and folds previous pending operation if applicable.
     * Uses left-to-right evaluation for "basic calculator" behavior.
     */
    void inputOperator(QChar op);

    // PUBLIC_INTERFACE
    /**
     * Evaluates with the current input and pending operation, returning the result as string.
     * If division by zero occurs, returns "Error" and resets pending operator.
     */
    QString evaluateEquals();

    // PUBLIC_INTERFACE
    /**
     * Returns the current input string being edited.
     */
    QString currentInput() const;

    // PUBLIC_INTERFACE
    /**
     * Returns the current accumulated result as string.
     */
    QString currentResult() const;

    // PUBLIC_INTERFACE
    /**
     * Basic helper for unit-like verification: computes left-to-right for a compact expression string
     * containing digits, decimal point and operators + - * / (no spaces). Returns "Error" on invalid/zero-div.
     */
    static QString computeExpressionLTR(const QString &expr, bool *ok = nullptr);

private:
    bool foldPending(); // folds m_pendingOp with m_accumulator and current input; returns false on error (e.g., div by zero)
    static bool isOperator(QChar c);

    QString m_current;      // current input being built
    double m_accumulator;   // accumulated result
    QChar m_pendingOp;      // '+', '-', '*', '/', or '\0'
    bool m_error;           // sticky error flag until clearAll()
};

/**
 * CalculatorWidget renders the calculator UI with Ocean Professional styling
 * and binds to CalculatorEngine for behavior.
 */
class CalculatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CalculatorWidget(QWidget *parent = nullptr);

private slots:
    void onDigitClicked();
    void onDecimalClicked();
    void onOperatorClicked();
    void onClearClicked();
    void onBackspaceClicked();
    void onEqualsClicked();

private:
    void buildUI();
    void applyTheme();
    void connectSignals();
    void refreshDisplay(const QString &secondaryHint = QString());
    QPushButton* makeButton(const QString &text, const QString &role);

    CalculatorEngine m_engine;

    // Display
    QLabel *m_resultLabel;   // Shows accumulated result or error
    QLineEdit *m_inputEdit;  // Shows current input

    // Buttons stored for potential styling/iteration
    QVector<QPushButton*> m_digitButtons;
    QPushButton *m_btnDecimal;
    QPushButton *m_btnAdd;
    QPushButton *m_btnSub;
    QPushButton *m_btnMul;
    QPushButton *m_btnDiv;
    QPushButton *m_btnClear;
    QPushButton *m_btnBack;
    QPushButton *m_btnEq;

    QGridLayout *m_grid;
};

#endif // MAIN_APP_H
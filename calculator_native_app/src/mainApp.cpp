#include "mainApp.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QRegularExpression>
#include <cmath>

// ---------------- CalculatorEngine ----------------

CalculatorEngine::CalculatorEngine()
    : m_current("0"), m_accumulator(0.0), m_pendingOp('\0'), m_error(false)
{
}

void CalculatorEngine::clearAll()
{
    m_current = "0";
    m_accumulator = 0.0;
    m_pendingOp = '\0';
    m_error = false;
}

void CalculatorEngine::backspace()
{
    if (m_error) { clearAll(); return; }
    if (m_current.size() <= 1) {
        m_current = "0";
        return;
    }
    m_current.chop(1);
    if (m_current == "-" || m_current == "-0") {
        m_current = "0";
    }
}

void CalculatorEngine::inputDigit(int d)
{
    if (m_error) { clearAll(); }
    if (d < 0 || d > 9) return;

    if (m_current == "0") {
        m_current = QString::number(d);
    } else if (m_current == "-0") {
        m_current = "-" + QString::number(d);
    } else {
        m_current += QString::number(d);
    }
}

void CalculatorEngine::inputDecimal()
{
    if (m_error) { clearAll(); }
    if (!m_current.contains('.')) {
        m_current += ".";
    }
}

bool CalculatorEngine::isOperator(QChar c)
{
    return c == '+' || c == '-' || c == '*' || c == '/';
}

bool CalculatorEngine::foldPending()
{
    // Convert current input to double
    bool ok = false;
    double rhs = m_current.toDouble(&ok);
    if (!ok) rhs = 0.0;

    if (m_pendingOp == '\0') {
        m_accumulator = rhs;
        return true;
    }

    // Perform operation left-to-right
    if (m_pendingOp == '+') {
        m_accumulator = m_accumulator + rhs;
    } else if (m_pendingOp == '-') {
        m_accumulator = m_accumulator - rhs;
    } else if (m_pendingOp == '*') {
        m_accumulator = m_accumulator * rhs;
    } else if (m_pendingOp == '/') {
        if (rhs == 0.0) {
            m_error = true;
            return false;
        }
        m_accumulator = m_accumulator / rhs;
    }
    return true;
}

void CalculatorEngine::inputOperator(QChar op)
{
    if (!isOperator(op)) return;
    if (m_error) { clearAll(); }

    // Fold current m_current into accumulator using pending
    if (!foldPending()) {
        // Division by zero
        m_pendingOp = '\0';
        m_current = "0";
        return;
    }

    // Set new pending operator and reset current input for next number
    m_pendingOp = op;
    m_current = "0";
}

QString CalculatorEngine::evaluateEquals()
{
    if (m_error) {
        clearAll();
        return QString("Error");
    }
    if (!foldPending()) {
        // Division by zero error
        QString out = "Error";
        // Reset states for next input, but keep error visible to caller
        clearAll();
        return out;
    }

    // Clear pending op; the result becomes current input for possible chaining
    m_pendingOp = '\0';
    QString resultStr = QString::number(m_accumulator, 'g', 12);
    // Normalize: if ends with .0-like for integers, convert to integer-looking representation
    // (Qt 'g' already does a good job; leave as is.)
    m_current = resultStr; // allow continued operations
    return resultStr;
}

QString CalculatorEngine::currentInput() const
{
    return m_current;
}

QString CalculatorEngine::currentResult() const
{
    if (m_error) return "Error";
    return QString::number(m_accumulator, 'g', 12);
}

QString CalculatorEngine::computeExpressionLTR(const QString &expr, bool *ok)
{
    if (ok) *ok = false;
    if (expr.isEmpty()) return "0";

    CalculatorEngine eng;
    eng.clearAll();

    QString num;
    auto flushNumber = [&]() {
        if (!num.isEmpty()) {
            // Push the number by simulating typing
            eng.m_current = num;
            num.clear();
        }
    };

    for (int i = 0; i < expr.size(); ++i) {
        QChar c = expr.at(i);
        if (c.isDigit()) {
            num.append(c);
        } else if (c == '.') {
            num.append(c);
        } else if (isOperator(c)) {
            flushNumber();
            eng.inputOperator(c);
            if (eng.m_error) return "Error";
        } else {
            return "Error"; // invalid char
        }
    }
    flushNumber();
    QString res = eng.evaluateEquals();
    if (res == "Error") return res;
    if (ok) *ok = true;
    return res;
}

// ---------------- CalculatorWidget ----------------

CalculatorWidget::CalculatorWidget(QWidget *parent)
    : QWidget(parent),
      m_resultLabel(new QLabel(this)),
      m_inputEdit(new QLineEdit(this)),
      m_btnDecimal(nullptr),
      m_btnAdd(nullptr),
      m_btnSub(nullptr),
      m_btnMul(nullptr),
      m_btnDiv(nullptr),
      m_btnClear(nullptr),
      m_btnBack(nullptr),
      m_btnEq(nullptr),
      m_grid(new QGridLayout())
{
    buildUI();
    applyTheme();
    connectSignals();
    refreshDisplay();
}

void CalculatorWidget::buildUI()
{
    setWindowTitle("Ocean Calculator");
    setMinimumSize(360, 520);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Display area
    m_resultLabel->setText("0");
    m_resultLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont resFont;
    resFont.setPointSize(12);
    resFont.setBold(true);
    m_resultLabel->setFont(resFont);
    m_resultLabel->setObjectName("resultLabel");

    m_inputEdit->setReadOnly(true);
    m_inputEdit->setAlignment(Qt::AlignRight);
    QFont inFont;
    inFont.setPointSize(24);
    inFont.setBold(true);
    m_inputEdit->setFont(inFont);
    m_inputEdit->setObjectName("inputEdit");

    auto *displayBox = new QVBoxLayout();
    displayBox->setSpacing(6);
    displayBox->addWidget(m_resultLabel);
    displayBox->addWidget(m_inputEdit);

    auto *displayWidget = new QWidget(this);
    displayWidget->setLayout(displayBox);
    displayWidget->setObjectName("displayWidget");

    root->addWidget(displayWidget);

    // Buttons grid
    QWidget *buttons = new QWidget(this);
    buttons->setLayout(m_grid);
    buttons->setObjectName("buttonsWidget");
    m_grid->setSpacing(10);

    // First row: C, ⌫, ÷
    m_btnClear = makeButton("C", "accent");
    m_btnBack  = makeButton("⌫", "accent");
    m_btnDiv   = makeButton("÷", "operator");
    m_grid->addWidget(m_btnClear, 0, 0, 1, 1);
    m_grid->addWidget(m_btnBack,  0, 1, 1, 1);
    m_grid->addWidget(m_btnDiv,   0, 2, 1, 1);

    // Digits 7 8 9 and ×
    QPushButton *b7 = makeButton("7", "digit");
    QPushButton *b8 = makeButton("8", "digit");
    QPushButton *b9 = makeButton("9", "digit");
    m_btnMul = makeButton("×", "operator");
    m_grid->addWidget(b7, 1, 0); m_digitButtons.push_back(b7);
    m_grid->addWidget(b8, 1, 1); m_digitButtons.push_back(b8);
    m_grid->addWidget(b9, 1, 2); m_digitButtons.push_back(b9);
    m_grid->addWidget(m_btnMul, 1, 3);

    // Digits 4 5 6 and -
    QPushButton *b4 = makeButton("4", "digit");
    QPushButton *b5 = makeButton("5", "digit");
    QPushButton *b6 = makeButton("6", "digit");
    m_btnSub = makeButton("−", "operator");
    m_grid->addWidget(b4, 2, 0); m_digitButtons.push_back(b4);
    m_grid->addWidget(b5, 2, 1); m_digitButtons.push_back(b5);
    m_grid->addWidget(b6, 2, 2); m_digitButtons.push_back(b6);
    m_grid->addWidget(m_btnSub, 2, 3);

    // Digits 1 2 3 and +
    QPushButton *b1 = makeButton("1", "digit");
    QPushButton *b2 = makeButton("2", "digit");
    QPushButton *b3 = makeButton("3", "digit");
    m_btnAdd = makeButton("+", "operator");
    m_grid->addWidget(b1, 3, 0); m_digitButtons.push_back(b1);
    m_grid->addWidget(b2, 3, 1); m_digitButtons.push_back(b2);
    m_grid->addWidget(b3, 3, 2); m_digitButtons.push_back(b3);
    m_grid->addWidget(m_btnAdd, 3, 3);

    // Row: 0 (span 2), ., =
    QPushButton *b0 = makeButton("0", "digit");
    m_btnDecimal = makeButton(".", "digit");
    m_btnEq = makeButton("=", "primary");
    m_grid->addWidget(b0, 4, 0, 1, 2); m_digitButtons.push_back(b0);
    m_grid->addWidget(m_btnDecimal, 4, 2, 1, 1);
    m_grid->addWidget(m_btnEq, 4, 3, 1, 1);

    // Move / operator to align like typical layout (place at column 3 in row 0)
    // Ensure ÷ at (0,3) with grid adjusting; we already placed ÷ at (0,2), so add a spacer and place properly:
    // Instead, extend first row to 4 columns: add a spacer at (0,2) and put ÷ at (0,3)
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_grid->addWidget(spacer, 0, 2);
    m_grid->addWidget(m_btnDiv, 0, 3);

    root->addWidget(buttons, 1);
}

void CalculatorWidget::applyTheme()
{
    // Base palette via stylesheet
    setStyleSheet(R"(
        QWidget {
            background-color: #f9fafb;
            color: #111827;
            font-family: 'Inter', 'Segoe UI', 'Helvetica Neue', Arial, sans-serif;
        }
        #displayWidget {
            background: #ffffff;
            border: 1px solid rgba(17,24,39,0.08);
            border-radius: 14px;
            padding: 12px;
        }
        #buttonsWidget {
            background: #ffffff;
            border: 1px solid rgba(17,24,39,0.08);
            border-radius: 14px;
            padding: 12px;
        }
        QLabel#resultLabel {
            color: rgba(17,24,39,0.7);
        }
        QLineEdit#inputEdit {
            background: #ffffff;
            border: none;
            color: #111827;
        }
        QPushButton {
            background: #ffffff;
            border: 1px solid rgba(37,99,235,0.15);
            border-radius: 12px;
            padding: 12px;
            font-size: 18px;
            min-width: 64px;
            min-height: 52px;
        }
        QPushButton:hover {
            background: #f3f6ff;
        }
        QPushButton[role="operator"] {
            background: #f0f6ff;
            color: #2563EB;
            border-color: rgba(37,99,235,0.35);
            font-weight: 600;
        }
        QPushButton[role="operator"]:hover {
            background: #e6efff;
        }
        QPushButton[role="accent"] {
            background: #fff8ed;
            color: #A16207;
            border-color: rgba(245,158,11,0.35);
            font-weight: 600;
        }
        QPushButton[role="accent"]:hover {
            background: #ffefd4;
        }
        QPushButton[role="primary"] {
            background: #2563EB;
            color: #ffffff;
            border: 1px solid #2563EB;
            font-weight: 700;
        }
        QPushButton[role="primary"]:hover {
            background: #1e4fd8;
        }
    )");
}

QPushButton* CalculatorWidget::makeButton(const QString &text, const QString &role)
{
    auto *btn = new QPushButton(text, this);
    btn->setProperty("role", role);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

void CalculatorWidget::connectSignals()
{
    // Digits
    for (auto *b : m_digitButtons) {
        connect(b, &QPushButton::clicked, this, &CalculatorWidget::onDigitClicked);
    }
    connect(m_btnDecimal, &QPushButton::clicked, this, &CalculatorWidget::onDecimalClicked);

    // Operators
    connect(m_btnAdd, &QPushButton::clicked, this, &CalculatorWidget::onOperatorClicked);
    connect(m_btnSub, &QPushButton::clicked, this, &CalculatorWidget::onOperatorClicked);
    connect(m_btnMul, &QPushButton::clicked, this, &CalculatorWidget::onOperatorClicked);
    connect(m_btnDiv, &QPushButton::clicked, this, &CalculatorWidget::onOperatorClicked);

    // Actions
    connect(m_btnClear, &QPushButton::clicked, this, &CalculatorWidget::onClearClicked);
    connect(m_btnBack,  &QPushButton::clicked, this, &CalculatorWidget::onBackspaceClicked);
    connect(m_btnEq,    &QPushButton::clicked, this, &CalculatorWidget::onEqualsClicked);
}

void CalculatorWidget::refreshDisplay(const QString &secondaryHint)
{
    // Update input
    m_inputEdit->setText(m_engine.currentInput());

    // Show accumulator or hint like "a +"
    if (!secondaryHint.isEmpty()) {
        m_resultLabel->setText(secondaryHint);
        return;
    }

    // Show pending op with accumulator if any
    QString hint;
    if (!m_engine.currentResult().isEmpty() && m_engine.currentResult() != "0") {
        hint = m_engine.currentResult();
    }
    m_resultLabel->setText(hint.isEmpty() ? "0" : hint);
}

void CalculatorWidget::onDigitClicked()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    bool ok = false;
    int d = btn->text().toInt(&ok);
    if (!ok && btn->text() == "0") d = 0;
    if (!ok && btn->text() != "0") return;

    m_engine.inputDigit(d);
    refreshDisplay();
}

void CalculatorWidget::onDecimalClicked()
{
    m_engine.inputDecimal();
    refreshDisplay();
}

void CalculatorWidget::onOperatorClicked()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString t = btn->text();
    QChar opChar = '\0';
    if (t == "+") opChar = '+';
    else if (t == "−" || t == "-") opChar = '-';
    else if (t == "×" || t == "*") opChar = '*';
    else if (t == "÷" || t == "/") opChar = '/';

    if (opChar == '\0') return;

    m_engine.inputOperator(opChar);
    QString hint = m_engine.currentResult();
    if (hint == "Error") {
        m_resultLabel->setText("Error");
    } else {
        // show "acc op"
        QString opStr = QString(" %1").arg(btn->text());
        m_resultLabel->setText(hint + opStr);
    }
    refreshDisplay(m_resultLabel->text());
}

void CalculatorWidget::onClearClicked()
{
    m_engine.clearAll();
    refreshDisplay();
}

void CalculatorWidget::onBackspaceClicked()
{
    m_engine.backspace();
    refreshDisplay();
}

void CalculatorWidget::onEqualsClicked()
{
    QString res = m_engine.evaluateEquals();
    if (res == "Error") {
        m_resultLabel->setText("Error");
        m_inputEdit->setText("0");
    } else {
        m_resultLabel->setText(res);
        m_inputEdit->setText(res);
    }
}

// ---------------- main ----------------

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CalculatorWidget calc;
    calc.show();

    // Lightweight in-code tests for core logic (non-exhaustive)
    {
        bool ok = false;
        QString r1 = CalculatorEngine::computeExpressionLTR("12+3*2", &ok); // LTR => (12+3)=15; 15*2=30
        Q_UNUSED(r1);
        Q_UNUSED(ok);
        // Division by zero -> Error
        QString r2 = CalculatorEngine::computeExpressionLTR("7/0");
        Q_UNUSED(r2);
    }

    return app.exec();
}

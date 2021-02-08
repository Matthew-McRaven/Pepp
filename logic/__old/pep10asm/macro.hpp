#pragma once

class MacroInvoke: public AsmCode
{
private:
    QStringList argumentList;
    QSharedPointer<ModuleInstance> macroInstance;
public:
    MacroInvoke() = default;
    ~MacroInvoke() override = default;
    MacroInvoke(const MacroInvoke& other);
    MacroInvoke& operator=(MacroInvoke other);
    AsmCode *cloneAsmCode() const override;
    void appendObjectCode(QList<int> &objectCode) const override;
    void adjustMemAddress(int addressDelta) override;

    // AsmCode interface
    QString getAssemblerListing() const override;
    QString getAssemblerSource() const override;
    quint16 objectCodeLength() const override;

    QStringList getArgumentList() const;
    void setArgumentList(QStringList);

    QSharedPointer<ModuleInstance> getMacroInstance() const;
    void setMacroInstance(QSharedPointer<ModuleInstance>);


    friend void swap(MacroInvoke& first, MacroInvoke& second)
    {
        using std::swap;
        swap(static_cast<AsmCode&>(first), static_cast<AsmCode&>(second));
        swap(first.argumentList, second.argumentList);
        swap(first.macroInstance, second.macroInstance);
    }
};
#include "macro.tpp"
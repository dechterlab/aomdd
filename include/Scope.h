#ifndef _SCOPE_H_
#define _SCOPE_H_

#include "base.h"

namespace aomdd {

class Scope {
protected:
    enum oper {
        UNION, INTERSECT, DIFF
    };

//    std::map<int, unsigned int> varCard;
    std::vector<unsigned int> varCard;
    std::list<int> ordering;

public:
    Scope();
    Scope(const Scope &s);

    // Provided the two scopes are compatible, perform the requested 
    // operation
    Scope(const Scope &lhs, const Scope &rhs, oper op);
    virtual ~Scope();

    // Add a variable if it does not already exist
    virtual bool AddVar(int i, unsigned int card);

    // Remove the variable
    virtual bool RemoveVar(int i);

    // Clear the scope
    virtual void Clear();

    // Check if a variable exists in the scope
    virtual bool VarExists(int i) const;

    virtual bool IsEmpty() const {
        return ordering.empty();
    }

    // Get the number of variables in the scope
    virtual unsigned int GetNumVars() const;

    // Get the cardinality of a variable
    virtual unsigned int GetVarCard(int i) const;

    // Get the cardinality of the scope
    virtual unsigned long GetCard() const;

    // Get the log cardinality of the scope
    virtual double GetLogCard() const;

    // Get the cardinality of the scope as a map of exponents
    virtual std::map<int, unsigned int> GetCardExp() const;

    // Check whether another scope has consistent cardinalities to this scope
    virtual bool HasConsistentCard(const Scope &rhs) const;

    // Get variable ordering
    virtual const std::list<int> &GetOrdering() const;

    // Set variable ordering
    virtual void SetOrdering(const std::list<int> &ordering);

    virtual Scope operator+(const Scope &rhs);
    virtual Scope operator*(const Scope &rhs);
    virtual Scope operator-(const Scope &rhs);

    virtual Scope& operator=(const Scope &rhs);

    virtual void Save(std::ostream &out) const;

    virtual unsigned long Mem() const {
        return varCard.size()*sizeof(unsigned int) + ordering.size()*sizeof(int);
    }

};

class Assignment: public Scope {
protected:
//    std::map<int, int> varAssigns;
    std::vector<int> varAssigns;
public:
    Assignment();
    Assignment(const Scope &s);
    Assignment(const Assignment &s);

    Assignment(const Assignment &lhs, const Assignment &rhs, oper op);

    virtual ~Assignment();

    // Add a variable if it does not already exist
    virtual bool AddVar(int i, unsigned int card);

    // Remove the variable
    virtual bool RemoveVar(int i);

    // Set a variables value
    virtual bool SetVal(int i, unsigned int val);

    // Set all variables to a specified value
    virtual void SetAllVal(unsigned int val);

    // Set values to match the assignment passed in
    virtual void SetAssign(const Assignment &a);

    // Unsets a value. (Sets it to UNKNOWN_VALUE)
    virtual bool UnsetVal(int i);

    virtual bool UnsetAllVal();

    // Retrieve the value of a variable
    virtual int GetVal(int i) const;

    // Get the index of the assignment in the scope
    virtual int GetIndex() const;

    // Get the index of the assignment under a different ordering
    virtual int GetIndex(const std::list<int> &otherOrder) const;

    // Iterates the assignment
    // Returns true if it iterates normally
    // Returns false if an unknown value is found or
    //   if the assignment wraps over
    virtual bool Iterate();

    // Set variable ordering
    virtual void SetOrdering(const std::list<int> &ordering);

    virtual Assignment operator+(const Assignment &rhs);
    virtual Assignment operator*(const Assignment &rhs);
    virtual Assignment operator-(const Assignment &rhs);

    virtual Assignment& operator=(const Assignment &a);

    void Save(std::ostream &out) const;

};

} // end of aomdd namespace

#endif

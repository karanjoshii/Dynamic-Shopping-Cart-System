#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <queue>

using namespace std;

class DiscountStrategy {
public:
    virtual ~DiscountStrategy() = default;
    virtual double calculate(double baseAmount) = 0;
};

class FlatDiscountStrategy : public DiscountStrategy {
private:
    double amount;
public:
    FlatDiscountStrategy(double amt) : amount(amt) {
        if (amt < 0) throw invalid_argument("Discount amount cannot be negative");
    }
    double calculate(double baseAmount) override {
        return min(amount, baseAmount);
    }
};

class PercentageDiscountStrategy : public DiscountStrategy {
private:
    double percent;
public:
    PercentageDiscountStrategy(double pct) : percent(pct) {
        if (pct < 0 || pct > 100) throw invalid_argument("Percentage must be between 0 and 100");
    }
    double calculate(double baseAmount) override {
        return (percent / 100.0) * baseAmount;
    }
};

class PercentageWithCapStrategy : public DiscountStrategy {
private:
    double percent;
    double cap;
public:
    PercentageWithCapStrategy(double pct, double capVal) : percent(pct), cap(capVal) {
        if (pct < 0 || pct > 100) throw invalid_argument("Percentage must be between 0 and 100");
        if (capVal < 0) throw invalid_argument("Cap cannot be negative");
    }
    double calculate(double baseAmount) override {
        double disc = (percent / 100.0) * baseAmount;
        return min(disc, cap);
    }
};

enum class StrategyType {
    FLAT,
    PERCENT,
    PERCENT_WITH_CAP
};

class DiscountStrategyManager {
private:
    static DiscountStrategyManager* instance;
    DiscountStrategyManager() = default;
public:
    static DiscountStrategyManager* getInstance() {
        if (!instance) {
            instance = new DiscountStrategyManager();
        }
        return instance;
    }
    unique_ptr<DiscountStrategy> getStrategy(StrategyType type, double param1, double param2 = 0.0) const {
        switch (type) {
            case StrategyType::FLAT:
                return make_unique<FlatDiscountStrategy>(param1);
            case StrategyType::PERCENT:
                return make_unique<PercentageDiscountStrategy>(param1);
            case StrategyType::PERCENT_WITH_CAP:
                return make_unique<PercentageWithCapStrategy>(param1, param2);
            default:
                throw invalid_argument("Invalid strategy type");
        }
    }
};
DiscountStrategyManager* DiscountStrategyManager::instance = nullptr;

class Product {
private:
    string name;
    string category;
    double price;
public:
    Product(string name, string category, double price) : name(name), category(category), price(price) {
        if (price < 0) throw invalid_argument("Price cannot be negative");
    }
    string getName() const { return name; }
    string getCategory() const { return category; }
    double getPrice() const { return price; }
    void setPrice(double newPrice) {
        if (newPrice < 0) throw invalid_argument("Price cannot be negative");
        price = newPrice;
    }
};

class CartItem {
private:
    const Product* product;
    int quantity;
public:
    CartItem(const Product* prod, int qty) : product(prod), quantity(qty) {
        if (qty <= 0) throw invalid_argument("Quantity must be positive");
    }
    double itemTotal() const { return product->getPrice() * quantity; }
    const Product* getProduct() const { return product; }
    int getQuantity() const { return quantity; }
    void setQuantity(int qty) {
        if (qty < 0) throw invalid_argument("Quantity cannot be negative");
        quantity = qty;
    }
};

class Cart {
private:
    unordered_map<const Product*, unique_ptr<CartItem>> items;
    double originalTotal;
    double currentTotal;
    bool loyaltyMember;
    string paymentBank;
public:
    Cart() : originalTotal(0.0), currentTotal(0.0), loyaltyMember(false), paymentBank("") {}
    ~Cart() = default;

    void addProduct(const Product* prod, int qty = 1) {
        if (items.find(prod) == items.end()) {
            items[prod] = make_unique<CartItem>(prod, qty);
        } else {
            items[prod]->setQuantity(items[prod]->getQuantity() + qty);
        }
        originalTotal += prod->getPrice() * qty;
        currentTotal += prod->getPrice() * qty;
    }

    void editItem(const Product* prod, int newQty) {
        if (items.find(prod) == items.end()) {
            throw invalid_argument("Product not in cart");
        }
        double oldTotal = items[prod]->itemTotal();
        if (newQty == 0) {
            items.erase(prod);
        } else {
            items[prod]->setQuantity(newQty);
        }
        recalculateTotals();
    }

    void recalculateTotals() {
        originalTotal = 0.0;
        currentTotal = 0.0;
        for (const auto& [prod, item] : items) {
            double total = item->itemTotal();
            originalTotal += total;
            currentTotal += total;
        }
    }

    double getOriginalTotal() const { return originalTotal; }
    double getCurrentTotal() const { return currentTotal; }
    void applyDiscount(double d) {
        if (d < 0) throw invalid_argument("Discount cannot be negative");
        currentTotal -= d;
        if (currentTotal < 0) currentTotal = 0;
    }
    void setLoyaltyMember(bool member) { loyaltyMember = member; }
    bool isLoyaltyMember() const { return loyaltyMember; }
    void setPaymentBank(string bank) { paymentBank = bank; }
    string getPaymentBank() const { return paymentBank; }
    const unordered_map<const Product*, unique_ptr<CartItem>>& getItems() const { return items; }
};

class Coupon {
private:
    Coupon* next;
public:
    Coupon() : next(nullptr) {}
    virtual ~Coupon() = default;
    void setNext(Coupon* nxt) {
        next = nxt;
        cout << "Setting next coupon for " << name() << " to " << (nxt ? nxt->name() : "null") << endl << flush;
    }
    Coupon* getNext() const { return next; }

    void applyDiscount(Cart* cart) {
        if (!cart) {
            cout << "Error: Cart is null in applyDiscount for " << name() << endl << flush;
            return;
        }
        cout << "Checking coupon: " << name() << endl << flush;
        if (isApplicable(cart)) {
            double discount = getDiscount(cart);
            cout << name() << " applied: " << fixed << setprecision(2) << discount << endl << flush;
            cart->applyDiscount(discount);
            if (!isCombinable()) {
                cout << name() << " is non-combinable, stopping chain." << endl << flush;
                return;
            }
        } else {
            cout << name() << " not applicable." << endl << flush;
        }
        if (next) {
            cout << "Moving to next coupon: " << next->name() << endl << flush;
            next->applyDiscount(cart);
        } else {
            cout << "End of coupon chain." << endl << flush;
        }
    }

    virtual bool isApplicable(const Cart* cart) const = 0;
    virtual double getDiscount(const Cart* cart) const = 0;
    virtual bool isCombinable() const { return true; }
    virtual string name() const = 0;
};

class SeasonalOffer : public Coupon {
private:
    double percent;
    string category;
    unique_ptr<DiscountStrategy> strat;
public:
    SeasonalOffer(double pct, string cat) : percent(pct), category(cat) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT, pct);
    }
    bool isApplicable(const Cart* cart) const override {
        for (const auto& [prod, item] : cart->getItems()) {
            if (item->getProduct()->getCategory() == category) return true;
        }
        return false;
    }
    double getDiscount(const Cart* cart) const override {
        double subtotal = 0.0;
        for (const auto& [prod, item] : cart->getItems()) {
            if (item->getProduct()->getCategory() == category) {
                subtotal += item->itemTotal();
            }
        }
        return strat->calculate(subtotal);
    }
    string name() const override {
        return "Seasonal Offer " + to_string(static_cast<int>(percent)) + "% off " + category;
    }
};

class LoyaltyDiscount : public Coupon {
private:
    double percent;
    unique_ptr<DiscountStrategy> strat;
public:
    LoyaltyDiscount(double pct) : percent(pct) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT, pct);
    }
    bool isApplicable(const Cart* cart) const override {
        return cart->isLoyaltyMember();
    }
    double getDiscount(const Cart* cart) const override {
        return strat->calculate(cart->getCurrentTotal());
    }
    string name() const override {
        return "Loyalty Discount " + to_string(static_cast<int>(percent)) + "% off";
    }
};

class BulkPurchaseDiscount : public Coupon {
private:
    double threshold;
    double flatOff;
    unique_ptr<DiscountStrategy> strat;
public:
    BulkPurchaseDiscount(double thr, double off) : threshold(thr), flatOff(off) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::FLAT, off);
    }
    bool isApplicable(const Cart* cart) const override {
        return cart->getOriginalTotal() >= threshold;
    }
    double getDiscount(const Cart* cart) const override {
        return strat->calculate(cart->getCurrentTotal());
    }
    bool isCombinable() const override { return false; }
    string name() const override {
        return "Bulk Purchase Rs " + to_string(static_cast<int>(flatOff)) + " off over " + to_string(static_cast<int>(threshold));
    }
};

class BankingCoupon : public Coupon {
private:
    string bank;
    double minSpend;
    double percent;
    double offCap;
    unique_ptr<DiscountStrategy> strat;
public:
    BankingCoupon(const string& b, double ms, double pct, double cap)
        : bank(b), minSpend(ms), percent(pct), offCap(cap) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT_WITH_CAP, pct, cap);
    }
    bool isApplicable(const Cart* cart) const override {
        return cart->getPaymentBank() == bank && cart->getOriginalTotal() >= minSpend;
    }
    double getDiscount(const Cart* cart) const override {
        return strat->calculate(cart->getCurrentTotal());
    }
    string name() const override {
        return bank + " Bank " + to_string(static_cast<int>(percent)) + "% off up to " + to_string(static_cast<int>(offCap));
    }
};

class CouponManager {
private:
    static CouponManager* instance;
    unordered_map<string, unique_ptr<Coupon>> coupons;
    vector<Coupon*> chain;
    CouponManager() = default;
public:
    static CouponManager* getInstance() {
        if (!instance) instance = new CouponManager();
        return instance;
    }
    void registerCoupon(unique_ptr<Coupon> coupon) {
        if (!coupon) {
            cout << "Error: Attempting to register null coupon" << endl << flush;
            return;
        }
        string name = coupon->name();
        cout << "Registering coupon: " << name << endl << flush;
        coupons[name] = move(coupon);
        Coupon* new_coupon = coupons[name].get();
        if (!new_coupon) {
            cout << "Error: Coupon pointer is null for " << name << endl << flush;
            return;
        }
        chain.push_back(new_coupon);
        cout << "Added " << name << " to chain (size: " << chain.size() << ")" << endl << flush;
        if (chain.size() > 1) {
            chain[chain.size() - 2]->setNext(new_coupon);
        }
    }
    vector<string> getApplicable(const Cart* cart) const {
        vector<string> res;
        for (const auto& [name, coupon] : coupons) {
            if (coupon && coupon->isApplicable(cart)) {
                res.push_back(name);
            }
        }
        return res;
    }
    double applyAll(Cart* cart) {
        cout << "Applying all discounts..." << endl << flush;
        if (!cart) {
            cout << "Error: Cart is null" << endl << flush;
            return 0.0;
        }
        struct CouponEntry {
            Coupon* coupon;
            double discount;
            bool operator<(const CouponEntry& other) const {
                return discount < other.discount;
            }
        };
        cart->recalculateTotals();
        priority_queue<CouponEntry> pq;
        for (const auto& [name, coupon] : coupons) {
            if (coupon->isApplicable(cart)) {
                pq.push({coupon.get(), coupon->getDiscount(cart)});
            }
        }
        if (pq.empty()) {
            cout << "No applicable coupons." << endl << flush;
            return cart->getCurrentTotal();
        }
        while (!pq.empty()) {
            auto entry = pq.top();
            pq.pop();
            if (entry.coupon->isApplicable(cart)) {
                double disc = entry.coupon->getDiscount(cart);
                cout << entry.coupon->name() << " applied: " << fixed << setprecision(2) << disc << endl << flush;
                cart->applyDiscount(disc);
                if (!entry.coupon->isCombinable()) {
                    cout << entry.coupon->name() << " is non-combinable, stopping." << endl << flush;
                    break;
                }
            }
        }
        return cart->getCurrentTotal();
    }
};
CouponManager* CouponManager::instance = nullptr;

int main() {
    try {
        CouponManager* mgr = CouponManager::getInstance();
        mgr->registerCoupon(make_unique<SeasonalOffer>(10, "Clothing"));
        mgr->registerCoupon(make_unique<LoyaltyDiscount>(5));
        mgr->registerCoupon(make_unique<BulkPurchaseDiscount>(10000, 500));
        mgr->registerCoupon(make_unique<BankingCoupon>("UPI", 2000, 15, 500));

        vector<unique_ptr<Product>> products;
        products.push_back(make_unique<Product>("Winter Jacket", "Clothing", 1000));
        products.push_back(make_unique<Product>("Smartphone", "Electronics", 20000));
        products.push_back(make_unique<Product>("Jeans", "Clothing", 1000));
        products.push_back(make_unique<Product>("Headphones", "Electronics", 2000));

        auto cart = make_unique<Cart>();

        while (true) {
            cout << "\n=== Shopping Cart Menu ===\n";
            cout << "1. Add Product to Cart\n";
            cout << "2. Edit Cart\n";
            cout << "3. Edit Product Price\n";
            cout << "4. Set Loyalty Status\n";
            cout << "5. Set Payment Bank\n";
            cout << "6. View Cart and Apply Discounts\n";
            cout << "7. Exit\n";
            cout << "Enter your choice (1-7): ";

            int choice;
            cin >> choice;
            if (cin.fail()) {
                cout << "Invalid input, please enter a number.\n";
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            cin.ignore();

            if (choice == 7) {
                cout << "Exiting program.\n";
                break;
            }

            switch (choice) {
                case 1: {
                    cout << "\nAvailable Products:\n";
                    for (size_t i = 0; i < products.size(); ++i) {
                        cout << i + 1 << ". " << products[i]->getName() 
                             << " (" << products[i]->getCategory() 
                             << ", " << fixed << setprecision(2) << products[i]->getPrice() << " Rs)\n";
                    }
                    cout << "Enter product number (1-" << products.size() << "): ";
                    int product_choice;
                    cin >> product_choice;
                    if (cin.fail() || product_choice < 1 || product_choice > static_cast<int>(products.size())) {
                        cout << "Invalid product number.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    cin.ignore();

                    cout << "Enter quantity: ";
                    int quantity;
                    cin >> quantity;
                    if (cin.fail() || quantity <= 0) {
                        cout << "Quantity must be positive.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    cin.ignore();

                    cart->addProduct(products[product_choice - 1].get(), quantity);
                    cout << "Added " << quantity << " x " << products[product_choice - 1]->getName() << " to cart.\n";
                    break;
                }
                case 2: {
                    if (cart->getItems().empty()) {
                        cout << "Cart is empty.\n";
                        break;
                    }
                    cout << "\nCart Items:\n";
                    int index = 1;
                    for (const auto& [prod, item] : cart->getItems()) {
                        cout << index++ << ". " << item->getProduct()->getName()
                             << " x " << item->getQuantity()
                             << " (Total: " << fixed << setprecision(2) << item->itemTotal() << " Rs)\n";
                    }
                    cout << "Enter item number to edit (1-" << cart->getItems().size() << "): ";
                    int item_index;
                    cin >> item_index;
                    if (cin.fail() || item_index < 1 || item_index > static_cast<int>(cart->getItems().size())) {
                        cout << "Invalid item number.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    cin.ignore();

                    const Product* selected_product = nullptr;
                    int current_index = 1;
                    for (const auto& [prod, item] : cart->getItems()) {
                        if (current_index++ == item_index) {
                            selected_product = prod;
                            break;
                        }
                    }

                    cout << "Enter new quantity (0 to remove): ";
                    int new_quantity;
                    cin >> new_quantity;
                    if (cin.fail() || new_quantity < 0) {
                        cout << "Invalid quantity.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    cin.ignore();

                    try {
                        cart->editItem(selected_product, new_quantity);
                        if (new_quantity == 0) {
                            cout << "Removed item from cart.\n";
                        } else {
                            cout << "Updated quantity to " << new_quantity << ".\n";
                        }
                    } catch (const exception& e) {
                        cout << "Error: " << e.what() << "\n";
                    }
                    break;
                }
                case 3: {
                    cout << "\nAvailable Products:\n";
                    for (size_t i = 0; i < products.size(); ++i) {
                        cout << i + 1 << ". " << products[i]->getName() 
                             << " (" << products[i]->getCategory() 
                             << ", " << fixed << setprecision(2) << products[i]->getPrice() << " Rs)\n";
                    }
                    cout << "Enter product number to edit price (1-" << products.size() << "): ";
                    int product_choice;
                    cin >> product_choice;
                    if (cin.fail() || product_choice < 1 || product_choice > static_cast<int>(products.size())) {
                        cout << "Invalid product number.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    cin.ignore();

                    cout << "Enter new price: ";
                    double new_price;
                    cin >> new_price;
                    if (cin.fail() || new_price < 0) {
                        cout << "Price must be non-negative.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    cin.ignore();

                    try {
                        products[product_choice - 1]->setPrice(new_price);
                        cart->recalculateTotals();
                        cout << "Updated price of " << products[product_choice - 1]->getName() << " to " << fixed << setprecision(2) << new_price << " Rs.\n";
                    } catch (const exception& e) {
                        cout << "Error: " << e.what() << "\n";
                    }
                    break;
                }
                case 4: {
                    cout << "\nAre you a loyalty member? (y/n): ";
                    char loyalty;
                    cin >> loyalty;
                    if (cin.fail()) {
                        cout << "Invalid input.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    cin.ignore();
                    cart->setLoyaltyMember(tolower(loyalty) == 'y');
                    cout << "Loyalty status set to: " << (cart->isLoyaltyMember() ? "Yes" : "No") << "\n";
                    break;
                }
                case 5: {
                    cout << "\nEnter payment bank (UPI,DEBIT CARD,CREDIT CARD): ";
                    string bank;
                    getline(cin, bank);
                    cart->setPaymentBank(bank);
                    cout << "Payment bank set to: " << bank << "\n";
                    break;
                }
                case 6: {
                    cout << "\nCart Items:\n";
                    if (cart->getItems().empty()) {
                        cout << " - Cart is empty\n";
                    } else {
                        for (const auto& [prod, item] : cart->getItems()) {
                            cout << " - " << item->getProduct()->getName()
                                 << " x " << item->getQuantity()
                                 << " (Total: " << fixed << setprecision(2) << item->itemTotal() << " Rs)\n";
                        }
                    }
                    cout << "Original Cart Total: " << fixed << setprecision(2) << cart->getOriginalTotal() << " Rs\n";
                    vector<string> applicable = mgr->getApplicable(cart.get());
                    cout << "Applicable Coupons:\n";
                    if (applicable.empty()) {
                        cout << " - None\n";
                    } else {
                        for (const auto& name : applicable) {
                            cout << " - " << name << "\n";
                        }
                    }
                    cout << "\nApplying discounts...\n" << flush;
                    double finalTotal = mgr->applyAll(cart.get());
                    cout << "Final Cart Total after discounts: " << fixed << setprecision(2) << finalTotal << " Rs\n";
                    break;
                }
                default:
                    cout << "Invalid choice. Please enter a number between 1 and 7.\n";
            }
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

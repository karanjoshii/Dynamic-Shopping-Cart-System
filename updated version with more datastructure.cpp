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
#include <stack>
#include <functional>
#include <unordered_set>
#include <limits> // For clearing input buffer

using namespace std;

// Defines a product with a name, category, and price.
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

// Represents a product and its quantity in the cart.
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

// Node for the Trie data structure.
class TrieNode {
public:
    unordered_map<char, unique_ptr<TrieNode>> children;
    bool isEndOfWord;
    TrieNode() : isEndOfWord(false) {}
};

// Trie for fast prefix-based string searches.
class Trie {
private:
    unique_ptr<TrieNode> root;
    // Helper to recursively find all words from a given node.
    void findWordsRecursive(TrieNode* node, string currentPrefix, vector<string>& results) {
        if (!node) return;
        if (node->isEndOfWord) { results.push_back(currentPrefix); }
        for (const auto& pair : node->children) {
            findWordsRecursive(pair.second.get(), currentPrefix + pair.first, results);
        }
    }
public:
    Trie() : root(make_unique<TrieNode>()) {}
    // Inserts a word into the Trie.
    void insert(const string& word) {
        TrieNode* current = root.get();
        for (char ch : word) {
            if (current->children.find(ch) == current->children.end()) {
                current->children[ch] = make_unique<TrieNode>();
            }
            current = current->children[ch].get();
        }
        current->isEndOfWord = true;
    }
    // Returns all words with a given prefix.
    vector<string> findWordsWithPrefix(const string& prefix) {
        vector<string> results;
        TrieNode* current = root.get();
        for (char ch : prefix) {
            if (current->children.find(ch) == current->children.end()) { return results; }
            current = current->children[ch].get();
        }
        findWordsRecursive(current, prefix, results);
        return results;
    }
};

// Singleton class to manage all available products.
class ProductCatalog {
private:
    static ProductCatalog* instance;
    unordered_map<string, unique_ptr<Product>> products;
    Trie productTrie;
    ProductCatalog() = default;
public:
    static ProductCatalog* getInstance() { if (!instance) instance = new ProductCatalog(); return instance; }

    void addProduct(string name, string category, double price) {
        products[name] = make_unique<Product>(name, category, price);
        productTrie.insert(name);
    }
    
    // Finds a product by its exact name.
    const Product* findProductByName(const string& name) const {
        auto it = products.find(name);
        return (it != products.end()) ? it->second.get() : nullptr;
    }
    
    // Provides auto-complete suggestions using the Trie.
    vector<string> getAutocompleteSuggestions(const string& prefix) {
        return productTrie.findWordsWithPrefix(prefix);
    }

    // Searches products using a flexible filter condition (lambda).
    vector<const Product*> searchProducts(function<bool(const Product&)> predicate) const {
        vector<const Product*> results;
        for (const auto& pair : products) {
            if (predicate(*(pair.second))) {
                results.push_back(pair.second.get());
            }
        }
        return results;
    }
};
ProductCatalog* ProductCatalog::instance = nullptr;

// Singleton class to manage product recommendations.
class RecommendationEngine {
private:
    static RecommendationEngine* instance;
    // Graph of product recommendations (adjacency list).
    unordered_map<const Product*, vector<const Product*>> recommendationsGraph;
    RecommendationEngine() = default;
public:
    static RecommendationEngine* getInstance() { if (!instance) instance = new RecommendationEngine(); return instance; }

    // Adds a two-way recommendation link between products.
    void addRecommendation(const Product* from, const Product* to) {
        recommendationsGraph[from].push_back(to);
        recommendationsGraph[to].push_back(from);
    }

    // Gets recommendations using a Breadth-First Search (BFS).
    vector<const Product*> getRecommendations(const Product* product, int maxCount) {
        vector<const Product*> results;
        if (recommendationsGraph.find(product) == recommendationsGraph.end()) { return results; }
        queue<const Product*> q;
        unordered_set<const Product*> visited;
        q.push(product);
        visited.insert(product);
        while (!q.empty() && results.size() < (size_t)maxCount) {
            const Product* current = q.front();
            q.pop();
            if (recommendationsGraph.count(current)) {
                for (const Product* neighbor : recommendationsGraph.at(current)) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        q.push(neighbor);
                        if (neighbor != product) {
                            results.push_back(neighbor);
                            if (results.size() == (size_t)maxCount) break;
                        }
                    }
                }
            }
        }
        return results;
    }
};
RecommendationEngine* RecommendationEngine::instance = nullptr;

// Pre-declaration of Cart for the Command class.
class Cart;

// Abstract base class for the Command pattern.
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
protected:
    Cart* cart;
    Command(Cart* c) : cart(c) {}
};

// Manages all items, totals, and user actions.
class Cart {
private:
    unordered_map<const Product*, unique_ptr<CartItem>> items;
    stack<unique_ptr<Command>> undoStack;
    stack<unique_ptr<Command>> redoStack;
    double originalTotal;
    double currentTotal;
    bool loyaltyMember;
    string paymentBank;
public:
    Cart() : originalTotal(0.0), currentTotal(0.0), loyaltyMember(false), paymentBank("") {}

    // Executes a command and saves it for undo.
    void executeCommand(unique_ptr<Command> command) {
        command->execute();
        undoStack.push(move(command));
        // Clear redo stack after a new action.
        stack<unique_ptr<Command>> empty;
        redoStack.swap(empty);
    }
    // Undoes the last command.
    void undo() {
        if (undoStack.empty()) { cout << "Nothing to undo.\n"; return; }
        unique_ptr<Command> command = move(undoStack.top());
        undoStack.pop();
        command->undo();
        redoStack.push(move(command));
    }
    // Re-applies the last undone command.
    void redo() {
        if (redoStack.empty()) { cout << "Nothing to redo.\n"; return; }
        unique_ptr<Command> command = move(redoStack.top());
        redoStack.pop();
        command->execute();
        undoStack.push(move(command));
    }
    // Adds a product or increases its quantity.
    void addProduct(const Product* prod, int qty = 1) {
        if (items.find(prod) == items.end()) { items[prod] = make_unique<CartItem>(prod, qty); } 
        else { items[prod]->setQuantity(items[prod]->getQuantity() + qty); }
        recalculateTotals();
    }
    // Changes a product's quantity or removes it.
    void editItem(const Product* prod, int newQty) {
        if (items.find(prod) == items.end() && newQty > 0) { 
            items[prod] = make_unique<CartItem>(prod, newQty); 
        } else if (items.find(prod) != items.end()) {
            if (newQty == 0) { items.erase(prod); }
            else { items[prod]->setQuantity(newQty); }
        }
        recalculateTotals();
    }
    // Gets the quantity of a specific product.
    int getQuantityOf(const Product* prod) const {
        auto it = items.find(prod);
        return (it != items.end()) ? it->second->getQuantity() : 0;
    }
    
    // Updates cart totals after any change.
    void recalculateTotals() {
        originalTotal = 0.0;
        currentTotal = 0.0;
        for (const auto& pair : items) {
            double total = pair.second->itemTotal();
            originalTotal += total;
            currentTotal += total;
        }
    }
    
    // Getters and setters for cart properties.
    double getOriginalTotal() const { return originalTotal; }
    double getCurrentTotal() const { return currentTotal; }
    void applyDiscount(double d) {
        if (d < 0) throw invalid_argument("Discount cannot be negative");
        currentTotal -= d;
        if (currentTotal < 0) currentTotal = 0;
    }
    void setLoyaltyMember(bool member) { loyaltyMember = member; }
    bool isLoyaltyMember() const { return loyaltyMember; }
    void setPaymentBank(const string& bank) { paymentBank = bank; }
    string getPaymentBank() const { return paymentBank; }
    const unordered_map<const Product*, unique_ptr<CartItem>>& getItems() const { return items; }
};

// Command to add a product to the cart.
class AddProductCommand : public Command {
private:
    const Product* product; 
    int quantity;
public:
    AddProductCommand(Cart* c, const Product* p, int qty) : Command(c), product(p), quantity(qty) {}
    void execute() override { cart->addProduct(product, quantity); }
    void undo() override {
        int currentQty = cart->getQuantityOf(product);
        cart->editItem(product, currentQty - quantity);
    }
};

// Command to change a product's quantity.
class EditItemCommand : public Command {
private:
    const Product* product; 
    int newQuantity; 
    int oldQuantity;
public:
    EditItemCommand(Cart* c, const Product* p, int nQty) : Command(c), product(p), newQuantity(nQty) {
        oldQuantity = cart->getQuantityOf(p);
    }
    void execute() override { cart->editItem(product, newQuantity); }
    void undo() override { cart->editItem(product, oldQuantity); }
};

// Interface for different discount calculation methods.
class DiscountStrategy {
public:
    virtual ~DiscountStrategy() = default;
    virtual double calculate(double baseAmount) = 0;
};

// Strategy for a fixed amount discount.
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

// Strategy for a percentage-based discount.
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

// Strategy for a percentage discount with a max cap.
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

// Enum to identify strategy types.
enum class StrategyType {
    FLAT,
    PERCENT,
    PERCENT_WITH_CAP
};

// Singleton factory to create discount strategy objects.
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
            case StrategyType::FLAT: return make_unique<FlatDiscountStrategy>(param1);
            case StrategyType::PERCENT: return make_unique<PercentageDiscountStrategy>(param1);
            case StrategyType::PERCENT_WITH_CAP: return make_unique<PercentageWithCapStrategy>(param1, param2);
            default: throw invalid_argument("Invalid strategy type");
        }
    }
};
DiscountStrategyManager* DiscountStrategyManager::instance = nullptr;

// Abstract base class for all coupons.
class Coupon {
public:
    virtual ~Coupon() = default;
    virtual bool isApplicable(const Cart* cart) const = 0;
    virtual double getDiscount(const Cart* cart) const = 0;
    virtual bool isCombinable() const { return true; }
    virtual string name() const = 0;
};

// Discount on products in a specific category.
class SeasonalOffer : public Coupon {
private:
    double percent; string category; unique_ptr<DiscountStrategy> strat;
public:
    SeasonalOffer(double pct, string cat) : percent(pct), category(cat) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT, pct);
    }
    bool isApplicable(const Cart* cart) const override {
        for (const auto& pair : cart->getItems()) {
            if (pair.second->getProduct()->getCategory() == category) return true;
        }
        return false;
    }
    double getDiscount(const Cart* cart) const override {
        double subtotal = 0.0;
        for (const auto& pair : cart->getItems()) {
            if (pair.second->getProduct()->getCategory() == category) {
                subtotal += pair.second->itemTotal();
            }
        }
        return strat->calculate(subtotal);
    }
    string name() const override {
        return "Seasonal Offer " + to_string(static_cast<int>(percent)) + "% off " + category;
    }
};

// Discount for loyalty program members.
class LoyaltyDiscount : public Coupon {
private:
    double percent; unique_ptr<DiscountStrategy> strat;
public:
    LoyaltyDiscount(double pct) : percent(pct) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT, pct);
    }
    bool isApplicable(const Cart* cart) const override { return cart->isLoyaltyMember(); }
    double getDiscount(const Cart* cart) const override { return strat->calculate(cart->getCurrentTotal()); }
    string name() const override { return "Loyalty Discount " + to_string(static_cast<int>(percent)) + "% off"; }
};

// Discount for orders over a certain total value.
class BulkPurchaseDiscount : public Coupon {
private:
    double threshold, flatOff; unique_ptr<DiscountStrategy> strat;
public:
    BulkPurchaseDiscount(double thr, double off) : threshold(thr), flatOff(off) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::FLAT, off);
    }
    bool isApplicable(const Cart* cart) const override { return cart->getOriginalTotal() >= threshold; }
    double getDiscount(const Cart* cart) const override { return strat->calculate(cart->getCurrentTotal()); }
    bool isCombinable() const override { return false; }
    string name() const override { return "Bulk Purchase Rs " + to_string(static_cast<int>(flatOff)) + " off over " + to_string(static_cast<int>(threshold)); }
};

// Discount for using a specific bank for payment.
class BankingCoupon : public Coupon {
private:
    string bank; double minSpend, percent, offCap; unique_ptr<DiscountStrategy> strat;
public:
    BankingCoupon(const string& b, double ms, double pct, double cap)
        : bank(b), minSpend(ms), percent(pct), offCap(cap) {
        strat = DiscountStrategyManager::getInstance()->getStrategy(StrategyType::PERCENT_WITH_CAP, pct, cap);
    }
    bool isApplicable(const Cart* cart) const override { return cart->getPaymentBank() == bank && cart->getOriginalTotal() >= minSpend; }
    double getDiscount(const Cart* cart) const override { return strat->calculate(cart->getCurrentTotal()); }
    string name() const override { return bank + " Bank " + to_string(static_cast<int>(percent)) + "% off up to " + to_string(static_cast<int>(offCap)); }
};

// Singleton class to manage and apply all coupons.
class CouponManager {
private:
    static CouponManager* instance;
    unordered_map<string, unique_ptr<Coupon>> coupons;
public:
    CouponManager() = default;
    static CouponManager* getInstance() { if (!instance) instance = new CouponManager(); return instance; }

    void registerCoupon(unique_ptr<Coupon> coupon) {
        if (!coupon) return;
        string n = coupon->name();
        coupons[n] = move(coupon);
    }
    
    // Finds and applies the best combination of discounts.
    double applyAll(Cart* cart) {
        if (!cart) return 0.0;
        struct CouponEntry {
            Coupon* coupon; double discount;
            bool operator<(const CouponEntry& other) const { return discount < other.discount; }
        };
        
        priority_queue<CouponEntry> pq;
        for (const auto& pair : coupons) {
            if (pair.second->isApplicable(cart)) {
                pq.push({pair.second.get(), pair.second->getDiscount(cart)});
            }
        }

        if (pq.empty()) {
            cout << "\nNo applicable coupons found." << endl;
            return cart->getCurrentTotal();
        }

        cout << "\nApplying best discounts...\n";
        while (!pq.empty()) {
            auto entry = pq.top();
            pq.pop();
            if (entry.coupon->isApplicable(cart)) {
                double disc = entry.coupon->getDiscount(cart);
                cout << " -> " << entry.coupon->name() << " applied: -" << fixed << setprecision(2) << disc << " Rs\n";
                cart->applyDiscount(disc);
                if (!entry.coupon->isCombinable()) {
                    cout << "   (This coupon is non-combinable, stopping further discounts)\n";
                    break;
                }
            }
        }
        return cart->getCurrentTotal();
    }
};
CouponManager* CouponManager::instance = nullptr;

// Main application entry point.
int main() {
    try {
        // Initialize all singleton managers.
        ProductCatalog* catalog = ProductCatalog::getInstance();
        RecommendationEngine* recommender = RecommendationEngine::getInstance();
        CouponManager* mgr = CouponManager::getInstance();

        // Add products to the catalog.
        catalog->addProduct("Winter Jacket", "Clothing", 1000);
        catalog->addProduct("Smartphone", "Electronics", 20000);
        catalog->addProduct("Smartwatch", "Electronics", 15000);
        catalog->addProduct("Jeans", "Clothing", 1000);
        catalog->addProduct("Headphones", "Electronics", 2000);
        catalog->addProduct("Wireless Headphones", "Electronics", 4500);

        // Define product recommendations.
        recommender->addRecommendation(catalog->findProductByName("Smartphone"), catalog->findProductByName("Smartwatch"));
        recommender->addRecommendation(catalog->findProductByName("Smartphone"), catalog->findProductByName("Wireless Headphones"));
        recommender->addRecommendation(catalog->findProductByName("Jeans"), catalog->findProductByName("Winter Jacket"));
        
        // Register available coupons.
        mgr->registerCoupon(make_unique<SeasonalOffer>(10, "Clothing"));
        mgr->registerCoupon(make_unique<LoyaltyDiscount>(5));
        mgr->registerCoupon(make_unique<BulkPurchaseDiscount>(25000, 1000));
        mgr->registerCoupon(make_unique<BankingCoupon>("UPI", 2000, 15, 500));

        auto cart = make_unique<Cart>();

        // Main application loop.
        while (true) {
            cout << "\n=========== Main Menu ===========\n"
                 << "1. Search Products\n"
                 << "2. Add Product to Cart\n"
                 << "3. Edit Cart Quantity\n"
                 << "4. View Recommendations\n"
                 << "5. Undo Last Action\n"
                 << "6. Redo Last Action\n"
                 << "7. Checkout (View Cart & Apply Discounts)\n"
                 << "8. Exit\n"
                 << "=================================\n"
                 << "Enter your choice: ";

            int choice;
            cin >> choice;
            // Handle invalid numerical input.
            if (cin.fail()) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number.\n";
                continue;
            }
            // Clear buffer for next getline input.
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            
            switch (choice) {
                case 1: { // Search Products
                    cout << "\n--- Product Search ---\n"
                         << "1. Search by Name (Autocomplete)\n"
                         << "2. Search by Category\n"
                         << "Enter search type: ";
                    int searchType;
                    cin >> searchType;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');

                    if (searchType == 1) {
                        cout << "Enter product name prefix: ";
                        string prefix;
                        getline(cin, prefix);
                        vector<string> suggestions = catalog->getAutocompleteSuggestions(prefix);
                        if (suggestions.empty()) {
                            cout << "No products found starting with '" << prefix << "'.\n";
                        } else {
                            cout << "Suggestions:\n";
                            for(const auto& name : suggestions) {
                                const Product* p = catalog->findProductByName(name);
                                cout << " - " << p->getName() << " (" << p->getCategory() << ", " << fixed << setprecision(2) << p->getPrice() << " Rs)\n";
                            }
                        }
                    } else if (searchType == 2) {
                        cout << "Enter category (e.g., Clothing, Electronics): ";
                        string category;
                        getline(cin, category);
                        vector<const Product*> results = catalog->searchProducts([&category](const Product& p){
                            string p_cat = p.getCategory();
                            // Perform case-insensitive comparison.
                            transform(p_cat.begin(), p_cat.end(), p_cat.begin(), ::tolower);
                            transform(category.begin(), category.end(), category.begin(), ::tolower);
                            return p_cat == category;
                        });
                        if(results.empty()){
                             cout << "No products found in category '" << category << "'.\n";
                        } else {
                            cout << "Products in '" << category << "':\n";
                            for(const auto* p : results){
                                cout << " - " << p->getName() << " (" << fixed << setprecision(2) << p->getPrice() << " Rs)\n";
                            }
                        }
                    } else {
                        cout << "Invalid search type.\n";
                    }
                    break;
                }
                case 2: { // Add Product to Cart
                    cout << "\n--- Add Product to Cart ---\n";
                    // Get all products from the catalog.
                    vector<const Product*> allProducts = catalog->searchProducts([](const Product& p){ return true; });
                    
                    if (allProducts.empty()) {
                        cout << "There are no products in the catalog.\n";
                        break;
                    }

                    // Display all available products for selection.
                    cout << "Available Products:\n";
                    for (size_t i = 0; i < allProducts.size(); ++i) {
                        cout << i + 1 << ". " << allProducts[i]->getName() 
                             << " (" << fixed << setprecision(2) << allProducts[i]->getPrice() << " Rs)\n";
                    }

                    // User selects a product by number.
                    cout << "Enter the number of the product to add: ";
                    int product_choice;
                    cin >> product_choice;
                    if (cin.fail() || product_choice < 1 || product_choice > (int)allProducts.size()) {
                        cout << "Invalid product number.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }

                    const Product* productToAdd = allProducts[product_choice - 1];
                    
                    // User provides the quantity.
                    cout << "Enter quantity for " << productToAdd->getName() << ": ";
                    int quantity;
                    cin >> quantity;
                     if (cin.fail() || quantity <= 0) {
                        cout << "Invalid quantity. Please enter a positive number.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    
                    // Execute the add command.
                    auto cmd = make_unique<AddProductCommand>(cart.get(), productToAdd, quantity);
                    cart->executeCommand(move(cmd));
                    cout << "Added " << quantity << " x " << productToAdd->getName() << " to cart.\n";
                    break;
                }
                case 3: { // Edit Cart Quantity
                    if (cart->getItems().empty()) {
                        cout << "Cart is empty. Nothing to edit.\n";
                        break;
                    }
                    cout << "\n--- Edit Cart Item ---\n";
                    vector<const Product*> cartProducts;
                    int idx = 1;
                    for(const auto& pair : cart->getItems()){
                        cout << idx++ << ". " << pair.first->getName() << " (Current Quantity: " << pair.second->getQuantity() << ")\n";
                        cartProducts.push_back(pair.first);
                    }
                    cout << "Enter item number to edit: ";
                    int item_choice;
                    cin >> item_choice;
                    if (cin.fail() || item_choice < 1 || item_choice > (int)cartProducts.size()) {
                        cout << "Invalid item number.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    
                    const Product* productToEdit = cartProducts[item_choice - 1];
                    cout << "Enter new quantity for " << productToEdit->getName() << " (0 to remove): ";
                    int new_quantity;
                    cin >> new_quantity;
                    if (cin.fail() || new_quantity < 0) {
                        cout << "Invalid quantity.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }

                    auto cmd = make_unique<EditItemCommand>(cart.get(), productToEdit, new_quantity);
                    cart->executeCommand(move(cmd));
                    cout << "Cart updated.\n";
                    break;
                }
                case 4: { // View Recommendations
                    if (cart->getItems().empty()) {
                        cout << "Add items to your cart to get recommendations.\n";
                        break;
                    }
                    cout << "\n--- Recommendations ---\n";
                    cout << "Get recommendations based on which product in your cart?\n";
                    vector<const Product*> cartProds;
                    int idx = 1;
                    for(const auto& pair : cart->getItems()){
                        cout << idx++ << ". " << pair.first->getName() << endl;
                        cartProds.push_back(pair.first);
                    }
                    cout << "Enter item number: ";
                    int rec_choice;
                    cin >> rec_choice;
                     if (cin.fail() || rec_choice < 1 || rec_choice > (int)cartProds.size()) {
                        cout << "Invalid choice.\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        break;
                    }
                    
                    const Product* selected_product = cartProds[rec_choice-1];
                    vector<const Product*> recs = recommender->getRecommendations(selected_product, 3);
                    
                    if (recs.empty()) {
                        cout << "No specific recommendations found for " << selected_product->getName() << ".\n";
                    } else {
                        cout << "You might also like:\n";
                        for (const auto* p : recs) {
                            cout << " - " << p->getName() << " (" << fixed << setprecision(2) << p->getPrice() << " Rs)\n";
                        }
                    }
                    break;
                }
                case 5: cart->undo(); break;
                case 6: cart->redo(); break;
                case 7: { // Checkout
                    cout << "\n--- Checkout Summary ---\n";
                    cart->recalculateTotals(); 
                    
                    cout << "Are you a loyalty member? (y/n): ";
                    char loyalty;
                    cin >> loyalty;
                    cart->setLoyaltyMember(tolower(loyalty) == 'y');
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    
                    cout << "Enter payment bank (e.g., UPI, HDFC): ";
                    string bank;
                    getline(cin, bank);
                    cart->setPaymentBank(bank);
                    
                    const auto& items = cart->getItems();
                    if (items.empty()) {
                        cout << "\nYour cart is empty.\n";
                    } else {
                        cout << "\nYour Cart:\n";
                        for (const auto& pair : items) {
                            cout << " - " << pair.second->getProduct()->getName() << " x " << pair.second->getQuantity() << "\n";
                        }
                    }
                    
                    cout << "\nOriginal Total: " << fixed << setprecision(2) << cart->getOriginalTotal() << " Rs\n";
                    
                    double finalTotal = mgr->applyAll(cart.get());
                    
                    cout << "\n-------------------------------------\n";
                    cout << "FINAL AMOUNT TO PAY: " << fixed << setprecision(2) << finalTotal << " Rs\n";
                    cout << "-------------------------------------\n";
                    break;
                }
                case 8: cout << "Exiting. Thank you for shopping!\n"; return 0;
                default: cout << "Invalid choice. Please try again.\n";
            }
        }
    } catch (const exception& e) {
        cerr << "An unexpected error occurred: " << e.what() << endl;
        return 1;
    }
    return 0;
}
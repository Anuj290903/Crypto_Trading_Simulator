#include "OrderBook.h"
#include "CSVReader.h"
#include "Wallet.h"
#include <map>
#include <algorithm>
#include <iostream>


/** construct, reading a csv data file */
OrderBook::OrderBook(std::string filename)
{
    orders = CSVReader::readCSV(filename);
}

/** return vector of all know products in the dataset*/
std::vector<std::string> OrderBook::getKnownProducts()
{
    std::vector<std::string> products;

    std::map<std::string,bool> prodMap;

    for (OrderBookEntry& e : orders)
    {
        prodMap[e.product] = true;
    }
    
    // now flatten the map to a vector of strings
    for (auto const& e : prodMap)
    {
        products.push_back(e.first);
    }

    return products;
}
/** return vector of Orders according to the sent filters*/
std::vector<OrderBookEntry> OrderBook::getOrders(OrderBookType type, 
                                        std::string product, 
                                        std::string timestamp)
{
    std::vector<OrderBookEntry> orders_sub;
    for (OrderBookEntry& e : orders)
    {
        if (e.orderType == type && 
            e.product == product && 
            e.timestamp == timestamp )
            {
                orders_sub.push_back(e);
            }
    }
    return orders_sub;
}


double OrderBook::getHighPrice(std::vector<OrderBookEntry>& orders)
{
    double max = orders[0].price;
    for (OrderBookEntry& e : orders)
    {
        if (e.price > max)max = e.price;
    }
    return max;
}


double OrderBook::getLowPrice(std::vector<OrderBookEntry>& orders)
{
    double min = orders[0].price;
    for (OrderBookEntry& e : orders)
    {
        if (e.price < min)min = e.price;
    }
    return min;
}

std::string OrderBook::getEarliestTime()
{
    return orders[0].timestamp;
}

std::string OrderBook::getNextTime(std::string timestamp)
{
    std::string next_timestamp = "";
    for (OrderBookEntry& e : orders)
    {
        if (e.timestamp > timestamp) 
        {
            next_timestamp = e.timestamp;
            break;
        }
    }
    if (next_timestamp == "")
    {
        next_timestamp = orders[0].timestamp;
    }
    return next_timestamp;
}

void OrderBook::insertOrder(OrderBookEntry& order)
{
    orders.push_back(order);
    std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
}

std::vector<OrderBookEntry> OrderBook::matchAsksToBids(std::string product, std::string timestamp, Wallet wallet)
{
    std::vector<OrderBookEntry> asks = getOrders(OrderBookType::ask, 
                                                 product, 
                                                 timestamp);

    std::vector<OrderBookEntry> bids = getOrders(OrderBookType::bid, 
                                                 product, 
                                                    timestamp);

    std::vector<OrderBookEntry> sales; 

    if (asks.size() == 0 || bids.size() == 0)
    {
        std::cout << " OrderBook::matchAsksToBids no bids or asks" << std::endl;
        return sales;
    }

    std::sort(asks.begin(), asks.end(), OrderBookEntry::compareByPriceAsc);
    std::sort(bids.begin(), bids.end(), OrderBookEntry::compareByPriceDesc);
    std::cout << "max ask " << asks[asks.size()-1].price << std::endl;
    std::cout << "min ask " << asks[0].price << std::endl;
    std::cout << "max bid " << bids[0].price << std::endl;
    std::cout << "min bid " << bids[bids.size()-1].price << std::endl;

    int i = 0, j = 0;
    while(i < asks.size())
    {
        OrderBookEntry& ask = asks[i];
        while(j < bids.size())
        {
            OrderBookEntry& bid = bids[j];
            if (bid.price >= ask.price)
            {
                OrderBookEntry sale{ask.price, 0, timestamp, 
                                    product, 
                                    OrderBookType::asksale};
                if (bid.username == "simuser")
                {
                    sale.username = "simuser";
                    if (!wallet.canFulfillOrder(bid))
                    {
                        j++;
                        std::cout << "OrderBook::matchAsksToBids Wallet cannot fulfill sale. Insufficient Balance" << std::endl;
                        continue;
                    }
                    sale.orderType = OrderBookType::bidsale;

                }
                if (ask.username == "simuser" && bid.username != "simuser")
                {
                    sale.username = "simuser";
                    if (!wallet.canFulfillOrder(ask))
                    {
                        std::cout << "OrderBook::matchAsksToBids Wallet cannot fulfill sale. Insufficient Balance" << std::endl;
                        break;
                    }
                    sale.orderType =  OrderBookType::asksale;
                }

                if (bid.amount == ask.amount)
                {
                    sale.amount = ask.amount;
                    sales.push_back(sale);
                    bid.amount = 0;
                    j++;
                    if (sale.username == "simuser")
                    {
                        wallet.processSale(sale);
                    }
                    break;
                }
                if (bid.amount > ask.amount)
                {
                    sale.amount = ask.amount;
                    sales.push_back(sale);
                    bid.amount =  bid.amount - ask.amount;
                    if (sale.username == "simuser")
                    {
                        wallet.processSale(sale);
                    }
                    break;
                }

                if (bid.amount < ask.amount && 
                   bid.amount > 0)
                {
                    sale.amount = bid.amount;
                    sales.push_back(sale);
                    ask.amount = ask.amount - bid.amount;
                    bid.amount = 0;
                    j++;
                    if (sale.username == "simuser")
                    {
                        wallet.processSale(sale);
                    }
                    continue;
                }
            }
            j++;
        }
        i++;
    }
    return sales;             
}

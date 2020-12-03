# Data Utility Functions

This is a repository with useful methods when working with data in C++. Most of them are analogous to some  python's pandas library functions.

# Examples

Given a structure representing the row of a dataframe and a vector with the data:

```
struct device
{
    std::string model;
    float buy_price;
    float sell_price;
};

std::vector<device> data;
```
## group_by

The following code groups the data by model:

```
auto groups = group_by(data, &device::model);
```
You can also group by more than one variable:

```
auto groups = group_by(data, &device::model, &device::buy_price);
```
## n_groups
Return the number of different groups.
```
auto n = n_groups(data, &device::model);

```
This is equivalent to (but faster):

```
auto n = group_by(data, &device::model).size();
```
## subset

Choose all observations where the buy price is more than 100: 

```
auto sub = subset(data, [](const device& r)
{ 
    return r.buy_price > 100;
});
```

## concat

Concatenate multiple data.
```
std::vector<device> a, b, c;
···
auto t = concat(a, b, c /*you can put more stuff here*/);
```

## min, max

`min` (and `max`) does not return the minimum (maximum) value, but instead it returns an iterator to the object where the element was found. 

Get the model of the device with minimum sell price:

```
auto model = min(data, &device::sell_price)->model;
```

## sort_asc, sort_des

`sort_asc`, `sort_des`  sort the data in ascending and descending order respectively. Data can be sorted by multiple variables:

```
sort_asc(data, &device::model, &device::sell_price)
```





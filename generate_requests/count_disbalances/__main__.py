import requests
import json
import time
import random

slicer_url = 'http://localhost:8080'

key_count = 1000
node_count = 20
nodes = []

current_nodes_to_ranges = []
current_metrics_with_host = []

def send_initialize_new_service_req(hosts):
    json_hosts = []
    for host in hosts:
        json_hosts.append({"Host" : host})
    json_data = {"New":{"Hosts" : json_hosts}}
    headers = {'Content-Type': 'application/json'}
    requests.post(slicer_url + '/api/v1/notify_nodes', data=json.dumps(json_data), headers=headers)

def get_node_mapping_request():
    response = requests.get(slicer_url + '/api/v1/get_mapping')
    json_response = response.json()
    rangesToNode = []
    for pair in json_response["RangeNodePairs"]:
        #print(pair)
        rangesToNode.append({
            "From" : pair["Range"]["From"],
            "To" : pair["Range"]["To"],
            "Host" : pair["Host"]})

    return rangesToNode

def send_upd_metrics_request(metrics):
    json_ranges = []
    for range in metrics:
        json_ranges.append({
            "Range" : {"From" : range["From"], "To" : range["To"]},
            "Value" : range["Value"]
        })

    json_data = {"Metrics" : json_ranges}
    headers = {'Content-Type': 'application/json'}
    requests.post(slicer_url + '/api/v1/send_metric', data=json.dumps(json_data), headers=headers)




def add_new_load_for_key(mapping_load_to_key):
    global current_nodes_to_ranges

    metrics = []
    metrics_host = []
    load_sum = {}
    for range in current_nodes_to_ranges:
        load_sum[range["To"]] = 0

    for key in mapping_load_to_key:
        for range in current_nodes_to_ranges:
            if range["From"] <= key and key <= range["To"]:
                load_sum[range["To"]] += mapping_load_to_key[key]
                break
    for range in current_nodes_to_ranges:
        #print("sum ", load_sum[range["To"]])
        metrics.append({"From" : range["From"], "To" : range["To"], "Value" : load_sum[range["To"]]})

    send_upd_metrics_request(metrics)


def generate_new_load():
    mapping_load_to_key = {}
    for key in range(key_count):
        load = random.random() * 100
        mapping_load_to_key[key] = load
    return mapping_load_to_key

def get_key_to_node():
    global current_nodes_to_ranges

    current_nodes_to_ranges = get_node_mapping_request()
    node_to_key = {}
    for node in range(node_count):
        node_to_key[str(node)] = []

    for range_to_node in current_nodes_to_ranges:
        for key in range(key_count):
            if key >= range_to_node["From"] and key <= range_to_node["To"]:
                node_to_key[range_to_node["Host"]].append(key)
    return node_to_key

def get_load_to_host(node_to_keys, current_load):
    node_to_load = {}
    for host in range(node_count):
        node_to_load[str(host)] = 0

    for node in node_to_keys:
        for key in node_to_keys[node]:
            node_to_load[node] += current_load[key]
    return node_to_load

def start_fake_service():
    for i in range(node_count):
        nodes.append(str(i))
    send_initialize_new_service_req(nodes)

def pretty_print_load(node_to_load):
    for node in node_to_load:
        print("node - ", node, "load - ", node_to_load[node])

def pretty_print_keys(node_to_load):
    for node in node_to_load:
        print("node - ", node, "keys - ", node_to_load[node])

def count_disbalance(node_to_load):
    su = 0
    cnt = 0
    ma = 0
    for node in node_to_load:
        su += node_to_load[node]
        cnt += 1
        ma = max(ma, node_to_load[node])
    return ma / (su / cnt)

if __name__ == "__main__":
    start_fake_service()
    current_load = generate_new_load()

    # print(node_to_keys)
    # print(current_load)
    #for i in range(10):
    node_to_keys = get_key_to_node()
    node_to_load = get_load_to_host(node_to_keys, current_load)

    pretty_print_keys(node_to_keys)
    pretty_print_load(node_to_load)
    #print(current_nodes_to_ranges)
    f = open('d.txt','w')
    time.sleep(2)

    for i in range(200):
        #for j in range(50):
        node_to_keys = get_key_to_node()
        node_to_load = get_load_to_host(node_to_keys, current_load)
        disb = count_disbalance(node_to_load)
        f.write(str(disb) + ", ")
        add_new_load_for_key(current_load)
        #print()
        time.sleep(2)

        # node_to_keys = get_key_to_node()
        # node_to_load = get_load_to_host(node_to_keys, current_load)

        # pretty_print_keys(node_to_keys)
        # pretty_print_load(node_to_load)

        # print(current_nodes_to_ranges)









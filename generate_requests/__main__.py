import requests
import json
import time

slicer_url = 'http://localhost:8081'

key_count = 1000
node_count = 10
nodes = []
current_nodes_to_ranges = []

def send_initialize_new_service_req(hosts):
    json_hosts = []
    for host in hosts:
        json_hosts.append({"Host" : host})
    json_data = {"New":{"Hosts" : json_hosts}}
    headers = {'Content-Type': 'application/json'}
    requests.post(slicer_url + '/api/v1/notify_nodes', data=json.dumps(json_data), headers=headers)

def get_node_mapping():
    response = requests.get(slicer_url + '/api/v1/get_mapping')
    json_response = response.json()
    rangesToNode = []
    for pair in json_response:
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

def add_new_host():

def delete_new_host():

def add_new_load_for_key(mapping_load_to_key):
    for

def start_fake_service():
    for i in range(key_count):
        nodes.append(str(i))
    send_initialize_new_service_req(nodes)


if __name__ == "__main__":
    start_fake_service()
    while (True):
        for i in range(1000):





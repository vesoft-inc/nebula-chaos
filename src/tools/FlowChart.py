from graphviz import Digraph
import json
import sys
import os

def parseJson(path):
    filename, _ = os.path.splitext(path)
    graph = Digraph(name = filename, format = "png", directory = ".")
    with open(path, 'r', encoding="utf-8") as f:
        data = json.load(f)

        # get all instance
        instances = data["instances"]

        # iterate all actions and draw node
        for i in range(len(data["actions"])):
            action = data["actions"][i]
            label = str(i) + ": " + action["type"]
            if "inst_index" in action:
                instance = instances[action["inst_index"]]
                label += "(" + instance["type"] + ")"
            graph.node(str(i), label)

        # iterate all actions and draw edges
        for i in range(len(data["actions"])):
            depends = data["actions"][i]["depends"]
            for depend in depends:
                graph.edge(str(depend), str(i))

        print(graph.render())

if __name__ == "__main__":
    if len(sys.argv) != 2 or sys.argv[1] == "-h":
        print("Install graphviz first: pip3 install graphviz")
        print("Or install from mirror: pip3 install -i https://pypi.tuna.tsinghua.edu.cn/simple graphviz")
        print("Use it like this: python3 FlowChart.py jsonPath")
    else:
        parseJson(sys.argv[1])

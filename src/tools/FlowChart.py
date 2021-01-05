from graphviz import Digraph
import json
import sys
import os

def parseActions(graph, instances, actions, prefix):
    # iterate all actions and draw node
    for i in range(len(actions)):
        action = actions[i]
        actionType = action["type"]
        label = str(i) + ": " + actionType

        # LoopAction need to draw subgraph
        if actionType == "LoopAction":
            # the subgraph name needs to begin with 'cluster' (all lowercase)
            # so that Graphviz recognizes it as a special cluster subgraph
            with graph.subgraph(name = "cluster" + str(i)) as sub:
                sub.attr(style = 'filled', color = 'lightgrey', label = label)
                parseActions(sub, instances, action["sub_plan"], str(i))

        if "inst_index" in action:
            instance = instances[action["inst_index"]]
            label += "(" + instance["type"] + ")"

        graph.node(prefix + str(i), label)


    # iterate all actions and draw edges
    for i in range(len(actions)):
        depends = actions[i]["depends"]
        for depend in depends:
            graph.edge(prefix + str(depend), prefix + str(i))


def parseJson(instancePath, actionPath):
    filename, _ = os.path.splitext(actionPath)
    graph = Digraph(name = filename, format = "png", directory = ".")
    with open(actionPath, 'r', encoding="utf-8") as actf:
        actiondata = json.load(actf)
        with open(instancePath, 'r', encoding="utf-8") as insf:
            instancedata = json.load(insf)
            # parse all actions
            parseActions(graph, instancedata["instances"], actiondata["actions"], "main")
            print(graph.render())

if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] == "-h":
        print("Install graphviz first: pip3 install graphviz")
        print("Or install from mirror: pip3 install -i https://pypi.tuna.tsinghua.edu.cn/simple graphviz")
        print("Use it like this: python3 FlowChart.py instanceJsonPath actionJsonPath")
    else:
        parseJson(sys.argv[1], sys.argv[2])

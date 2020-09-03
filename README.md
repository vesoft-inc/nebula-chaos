# nebula-chaos
Chaos framework for the Storage Service

### Plan Intro
There are some built-in plans in nebula-chaos. Each plan is a json in [conf](conf/) directory. The plan need to specify some instances (usually including nebula graph/meta/storage) and some actions. The actions is a collection of different type actions, which forms a dag. The dependency between actions need to be specified in `depends` field. Most of the action need to specify related nebula instance in `inst_index` field. You can add customize based on these rules.

A utils to draw a flow chart of the plan is included, use it like this: `python3 src/tools/FlowChart.py conf/scale_up_and_down.json`.

#### [checkpoint_create_restore](conf/checkpoint_create_restore_plan.json)
Start all services, write data, then create a check point, write some more data, restore from check point. In the end, we check the validity by checking whether data is the same as the one when we create check point.

#### [clean_wals_restart](conf/clean_wals_restart.json)
Clean all wals of specified space, then start all services, write a circle, then check data integrity.

#### [random_kill_clean_data](conf/random_kill_clean_data_plan.json)
Start all services, disturb (random kill a storage service, clean the data path, restart) while write a circle, then check data integrity.

#### [random_kill](conf/random_kill_plan.json)
Start all services, disturb (random kill and restart a storage service) while write and read.

#### [kill_all](conf/kill_all_plan.json)
Start all services, kill all storage services and restart while writing.

#### [scale_up_and_down](conf/scale_up_and_down.json)
Start 3 storage servies, add 4th storage service using `balance data` while write a circle, then check data integrity. Then stop 1st storage service, remove it using `balance data` while write a circle then check data integrity. Likewise,
add 1st storage service back and remove the 4th storage service.

#### [loop_scale_up_and_down](conf/loop_scale_up_and_down.json)
Similar to `scale_up_and_down`, but loop for serveral times. Loop once takes about 15 mins which may vary according to the environment, you can adjust the loop times by yourself.

#### [random_network_partition](conf/random_network_partition.json)
Start all services, disturb (random drop all packets of a storage service, recover later) while write a circle, then check data integrity. The network partition is based on iptables. **Make sure the user has sudo authority and can execute iptables without password.**

> PS: all storage services in [random_network_partition](conf/random_network_partition.json) and [random_traffic_control](conf/random_traffic_control.json) must be deployed on different ip. The reason is that we don't know the source port of storage service, we can only use ip to indicate the service.

#### [random_traffic_control](conf/random_traffic_control.json)
Start all services, disturb (random delay all packets of a storage service, recover later) while write a circle, then check data integrity. The traffic is based on [tcconfig](https://github.com/thombashi/tcconfig), which is a `tc` command wrapper. Install it at first, since it will use `tc` and `ip` command, use the following scripts to make it has capabilities with not super user.
```
setcap cap_net_admin+ep /usr/sbin/tc
setcap cap_net_raw,cap_net_admin+ep /usr/sbin/ip
```

#### [random_disk_full](conf/random_disk_full.json)
Start all services, disturb (cat /dev/zero until disk is full) while write a circle, the storage services which use the direcory should be crashed, then we clean the mock file and restart, check data integrity at last.

**Use a ramdisk or tmpfs with limited size to test this plan, otherwise the whole disk will be occupied.**

#### [random_slow_disk](conf/random_slow_disk.json)
Start all services, disturb (simulate slow disk io) while write a circle, then check data integrity. We use [SysytemTap](https://sourceware.org/systemtap/wiki) to simulate slow disk io. The `major` and `minor` field is the MAJOR/MINOR device id of disk where storage serveice's data path mounted.

```
yum install systemtap
```
You may need install `kernel-devel` and `kernel-debuginfo` as well (the version must be same with kernel).
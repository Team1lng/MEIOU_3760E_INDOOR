#!/usr/bin/env python3
import sys
import yaml

def get_yaml_data(yaml_file,output):
    with open(yaml_file,'r')as file:
        content = file.read()
        file.close()

        data = yaml.load(content, Loader=yaml.FullLoader)

        name = data.get('info').get('name')
        version= data.get('info').get('version')

        if len(vendor_sdk) and name == vendor_sdk:
                   print(name,version)
                   with open(prop_file,'a') as f:
                        f.write('persist.%s.version=%s\n' % (name, version))
                   f.close()

        componnent=data.get('dependencies').get('components');

        with open(output,'w') as p:
            p.write('[version]\n')

            for i in range(len(componnent)):
               name_group = ['boot','linux','board']
               name = componnent[i].get('Component').get('name')
               version = componnent[i].get('Component').get('version')

               for j in range(len(name_group)):
                   if name.find(name_group[j]) >= 0:
                        print(name,version)
                        if name_group[j] == "board":
                            name_group[j] = "rootfs"

                        if name_group[j] == "linux":
                            name_group[j] = "kernel"

                        p.write('%s = %s\n' % (name_group[j],version))
                        continue
               
        
        p.close()

if __name__ == "__main__":

    if len(sys.argv) < 5:
        print("%s %s %s\n" %(sys.argv[0],"yaml_file","output_file",'prop_file','vendor_sdk'))
        sys.exit(1)

    prop_file = sys.argv[3]
    vendor_sdk = sys.argv[4]

    print(sys.argv[1])
    print(sys.argv[2])
    print(sys.argv[3])
    print(sys.argv[4])

    get_yaml_data(sys.argv[1],sys.argv[2])

    sys.exit(0)

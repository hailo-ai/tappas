#!/usr/bin/env python3
import os
import subprocess
import re
import sys

# Matches substrings of the form: `Title <path_to_rst.rst>`_
RST_LINK_REGEX = r"\`[^\`]*?<(?!(http)).*?\.rst>\`\_"

# (`[^`]*?<.*?\.rst)(#.*?)(>`_) --> Matches the form: `Title <path_to_rst.rst#label_in_the_rst_file>`_
RST_LINK_HASH_REGEX_SED = "\\(\\`[^\\`]*\\?<.*\\?\\.rst\\)\\(\\#.*\\?\\)\\(>\\`\\_\\)"
# Change substrings of the form of RST_LINK_HASH_REGEX_SED to: `Title <path_to_rst.rst>`_
LINK_HASH_SPHINX = '\\1\\3'

# "(\`)([^\`]*?)(\.rst)(>.)(\_)" --> Matches the form: `Title <path_to_rst.rst>`_
RST_LINK_REGEX_SED = "\\(\\`\\)\\([^\\`]*\\?\\)\\(\\.rst\\)\\(>.\\)\\(\\_\\)"
# Change substrings of the form of RST_LINK_REGEX_SED to: :ref:`Title <path_to_rst>`
LINK_REF_SPHINX = ':ref:\\1\\2\\4'


def insert_first_line(file_relative_path):
    first_line = f'.. _{file_relative_path}:'
    subprocess.call(["sed", "-i", f'1i {first_line.replace(".rst", "")}', file_relative_path])
    subprocess.call(["sed", "-i", '2i \n', file_relative_path])

def fix_hash_links(file_relative_path):
    sed_hash_links = f'sed -i "s+{RST_LINK_HASH_REGEX_SED}+{LINK_HASH_SPHINX}+g" {file_relative_path}'
    subprocess.run(sed_hash_links, shell=True) 

def find_all_links_in_file(file_relative_path):
    find_links_command = ["grep", "-Po", RST_LINK_REGEX, file_relative_path]
    find_links_process = subprocess.run(find_links_command, stdout=subprocess.PIPE)
    rst_links_in_file = find_links_process.stdout.decode(sys.stdout.encoding).split('\n')
    return list(set(rst_links_in_file))

def get_path_from_link(link_to_rst):
    return re.search(r"<.*>", link_to_rst).group(0)[1:-1]

def get_relative_paths_for_all_links(list_of_links, file_full_path):
    paths_to_convert = []
    for link_to_rst in list_of_links:
        if link_to_rst:
            path_in_file = get_path_from_link(link_to_rst)
            full_path_to_insert = os.path.join(os.path.dirname(file_full_path), path_in_file)

            if os.path.exists(full_path_to_insert):
                path_to_insert = os.path.relpath(full_path_to_insert)
                if path_in_file != path_to_insert:                    
                    paths_to_convert.append((path_in_file, path_to_insert))                                        
            else:
                error_message = f"Link in {file_full_path} to file {full_path_to_insert} does not exist."
                raise FileNotFoundError(error_message) 
                
    return list(set(paths_to_convert))

def convert_to_relative_path(paths_to_convert, file_relative_path):
    for path_in_file, path_to_insert in paths_to_convert:
        sed_fix_path = f"sed -i 's+<{path_in_file}>+<{path_to_insert}>+g' {file_relative_path}"
        subprocess.run(sed_fix_path, shell=True)    

def set_ref_in_file(file_relative_path):
    sed_ref = f'sed -i "s+{RST_LINK_REGEX_SED}+{LINK_REF_SPHINX}+g" {file_relative_path}'
    subprocess.run(sed_ref, shell=True) 

def main():
    start = os.path.dirname(os.path.realpath(__file__))

    for root, dirs, files in os.walk(start):
        for file in files:
            if file.endswith('.rst'):

                file_full_path = os.path.realpath(f'{root}/{file}')
                file_relative_path = os.path.relpath(file_full_path)
                
                insert_first_line(file_relative_path)
                fix_hash_links(file_relative_path)

                rst_links_in_file = find_all_links_in_file(file_relative_path)
                paths_to_convert = get_relative_paths_for_all_links(rst_links_in_file, file_full_path)
                
                convert_to_relative_path(paths_to_convert, file_relative_path)
                set_ref_in_file(file_relative_path)
                        
if __name__ == '__main__':
    main()
                   
                    

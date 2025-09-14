def write_html(path,file,var_name):
    with open(path, "rb") as f:
        data = f.read()

    c_array = ", ".join(f"0x{b:02x}" for b in data)
    c_code = f"unsigned char {var_name}_data[] = {{ {c_array} }};\nunsigned int {var_name}_data_len = {len(data)};"

    with open(file, "w") as f:
        f.write(c_code)



write_html("web/dist/index.html","service/html.h","html")

write_html("web/src/assets/countries.geojson","service/geojson.h","geojson")

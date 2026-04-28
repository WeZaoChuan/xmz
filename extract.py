import pdfplumber
import json
import re
import os

pdf_path = 'book.pdf'
output_json = 'data.json'

def extract_school_data():
    if not os.path.exists(pdf_path): return
    print("常用（全能先知）启动【V7 级原子化拆解】，正在为您剥离招生详情子模块...")
    
    schools_data = []
    current_school = None
    current_sec = None      # 大模块：二、三、四、五
    current_sub_sec = None  # 招生信息内的子模块：1.目录 2.计算 3.分数线

    try:
        with pdfplumber.open(pdf_path) as pdf:
            for page in pdf.pages:
                tables = page.extract_tables()
                text = page.extract_text()
                if not text: continue
                lines = text.split('\n')
                
                for line in lines:
                    line = line.strip()
                    if not line: continue
                    
                    # 1. 识别新学校
                    match = re.match(r'^(\d{5})\s+(.+)$', line)
                    if match and len(match.group(2)) <= 30 and '奖学金' not in line:
                        if current_school: schools_data.append(current_school)
                        current_school = {
                            "code": match.group(1), "name": match.group(2).strip(),
                            "level": "未知", "location": "未知", "difficulty": "未知", "rank": "未知", "property": "未知",
                            "admission_catalog": "", "admission_score": "", "admission_lines": "", # 拆分招生信息
                            "sec_scholarship": "", "sec_advice": "", "sec_contact": ""
                        }
                        current_sec = None
                        current_sub_sec = None
                        continue

                    if current_school:
                        # 2. 基础信息抓取
                        if "学校层次：" in line: current_school["level"] = line.split("：")[-1]
                        elif "所属省市：" in line: current_school["location"] = line.split("：")[-1]
                        elif "考研难度：" in line: current_school["difficulty"] = line.split("：")[-1]
                        elif "院校属性：" in line: current_school["property"] = line.split("：")[-1]
                        elif "排名：" in line: current_school["rank"] = line.split("：")[-1]

                        # 3. 招生信息子分级 (1. 2. 3. 识别)
                        if "招生目录" in line and ("1." in line or "一、" in line):
                            current_sec = "admission"; current_sub_sec = "admission_catalog"
                        elif "总成绩计算" in line and ("2." in line or "二、" in line):
                            current_sec = "admission"; current_sub_sec = "admission_score"
                        elif "录取分数线" in line and ("3." in line or "三、" in line):
                            current_sec = "admission"; current_sub_sec = "admission_lines"
                        
                        # 4. 其他大模块
                        elif "三、奖学金" in line: current_sec = "sec_scholarship"; current_sub_sec = None
                        elif "四、报考建议" in line: current_sec = "sec_advice"; current_sub_sec = None
                        elif "五、相关信息" in line: current_sec = "sec_contact"; current_sub_sec = None
                        
                        # 5. 内容填充
                        if current_sec == "admission" and current_sub_sec:
                            if len(line) > 10: current_school[current_sub_sec] += f"<p>{line}</p>"
                        elif current_sec and current_sec.startswith("sec_"):
                            if len(line) > 10: current_school[current_sec] += f"<p>{line}</p>"

                # 6. 表格精准归类（招生目录表格）
                if current_school and tables:
                    for table in tables:
                        html_table = "<div class='table-wrapper'><table>"
                        for r_idx, row in enumerate(table):
                            row_text = "".join([str(c) for c in row if c])
                            # 根据表格内容特征，判断归入哪个子模块
                            target = "admission_catalog" if "专业" in row_text or "科目" in row_text else "admission_lines"
                            clean_row = [str(cell).replace('\n', '<br>') if cell else '-' for cell in row]
                            html_table += "<tr>" + "".join([f"<td>{c}</td>" for c in clean_row]) + "</tr>"
                        html_table += "</table></div>"
                        current_school[target] += html_table

            if current_school: schools_data.append(current_school)
        with open(output_json, 'w', encoding='utf-8') as f:
            json.dump(schools_data, f, ensure_ascii=False, indent=4)
        print("【V7 数据拆解完成】")
    except Exception as e: print(f"错误: {e}")

if __name__ == "__main__": extract_school_data()
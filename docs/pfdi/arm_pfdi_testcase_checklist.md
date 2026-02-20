## PFDI ACS Testcase checklist

The PFDI ACS test checklist is based on **PFDI 1.0 BET1 specification** and **PFDI ACS 0.9.0** tag.

The checklist provides information about:

1. The PFDI rules covered by each test.
2. The runtime environment is UEFI in which each test executes.

[Latest checklist changes](#latest-checklist-changes) summarizing the latest checklist changes relative to the latest released tag, is present at end of document.

<br>
<table border="1" class="dataframe dataframe">
  <thead>
    <tr style="text-align: right;">
      <th>Specification Checklist Rule</th>
      <th>Test ID</th>
      <th>Test Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>R0053</td>
      <td>1</td>
      <td>Check PFDI Version is returned</td>
    </tr>
    <tr>
      <td>R0104</td>
      <td>2</td>
      <td>Check PFDI Version in All PE's</td>
    </tr>
    <tr>
      <td>R0102</td>
      <td>3</td>
      <td>Check PFDI mandatory functions</td>
    </tr>
    <tr>
      <td>R0060</td>
      <td>4</td>
      <td>Check PFDI Feature function support</td>
    </tr>
    <tr>
      <td>R0066</td>
      <td>5</td>
      <td>Check PE HW test mechanism info</td>
    </tr>
    <tr>
      <td>R0071</td>
      <td>6</td>
      <td>Check num of Test Part supported</td>
    </tr>
    <tr>
      <td>R0076</td>
      <td>7</td>
      <td>Execute Test Parts and All Parts on PE</td>
    </tr>
    <tr>
      <td>R0082</td>
      <td>8</td>
      <td>Query PE boot test status</td>
    </tr>
    <tr>
      <td>R0089</td>
      <td>9</td>
      <td>Query PFDI firmware check on all PEs</td>
    </tr>
    <tr>
      <td>R0156</td>
      <td>10</td>
      <td>PFDI reserved function support check</td>
    </tr>
    <tr>
      <td>R0040</td>
      <td>11</td>
      <td>Check if X5 to X17 are preserved</td>
    </tr>
    <tr>
      <td>R0099</td>
      <td>12</td>
      <td>PFDI forced error injection</td>
    </tr>
    <tr>
      <td>R0164</td>
      <td>13</td>
      <td>Check PE Run with Start exceeds End</td>
    </tr>
    <tr>
      <td>R0165</td>
      <td>14</td>
      <td>Check PE Run with Start exceeds max</td>
    </tr>
    <tr>
      <td>R0100</td>
      <td>15</td>
      <td>PFDI recovery after forced error</td>
    </tr>
    <tr>
      <td>R0157</td>
      <td>16</td>
      <td>Check PFDI feature for invalid function</td>
    </tr>
    <tr>
      <td>R0154</td>
      <td>17</td>
      <td>Check PFDI unsupported function</td>
    </tr>
    <tr>
      <td>R0166</td>
      <td>18</td>
      <td>Check PE Run with End exceeds max index</td>
    </tr>
    <tr>
      <td>R0167</td>
      <td>19</td>
      <td>Check PE Run with Start or End equals -1</td>
    </tr>
    <tr>
      <td>R0168</td>
      <td>20</td>
      <td>Check PE Run with Start or End less -1</td>
    </tr>
    <tr>
      <td>R0155</td>
      <td>21</td>
      <td>PFDI version invalid params check</td>
    </tr>
    <tr>
      <td>R0179</td>
      <td>22</td>
      <td>PFDI Feature invalid params check</td>
    </tr>
    <tr>
      <td>R0178</td>
      <td>23</td>
      <td>PFDI PE Test ID invalid params check</td>
    </tr>
    <tr>
      <td>R0160</td>
      <td>24</td>
      <td>PFDI Test Part Count invalid params check</td>
    </tr>
    <tr>
      <td>R0172</td>
      <td>25</td>
      <td>PFDI PE Test Result invalid params check</td>
    </tr>
    <tr>
      <td>R0173</td>
      <td>26</td>
      <td>PFDI Firmware Check invalid params</td>
    </tr>
    <tr>
      <td>R0193</td>
      <td>27</td>
      <td>Check FORCE ERROR overwrite behavior</td>
    </tr>
    <tr>
      <td>R0194</td>
      <td>28</td>
      <td>Check FORCE ERROR PE locality behavior</td>
    </tr>
    <tr>
      <td>R0163</td>
      <td>29</td>
      <td>PFDI Test Run invalid params check</td>
    </tr>
    <tr>
      <td>R0180</td>
      <td>30</td>
      <td>PFDI Force Error invalid params check</td>
    </tr>
    <tr>
      <td>R0176</td>
      <td>31</td>
      <td>PFDI Force Error invalid function ID check</td>
    </tr>
  </tbody>
</table>

## Latest Checklist Changes
- Updated Checklist as per PFDI 1.0 BET1 Specification.
